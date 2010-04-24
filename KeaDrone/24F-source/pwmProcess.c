#include "GenericTypeDefs.h"
#include "Compiler.h"

#include "pwmHandler.h"

/* If high, the keypad-light will be
 * continuous on instead of dimming value*/
static volatile UINT8 kpLightOverrideOn = 0;
/* if low, all outputs will be offline */
static volatile UINT8 kpLightOverrideOff = 0;

/* It is possible to generate more than 1
 * PWM output with different Duty*/
static volatile UINT8 dutyCycle1 = 0;
static volatile UINT8 dutyCycle2 = 0;

#if defined (__PIC24F__)
/* Interrupt vector for the UART -RX */
void __attribute__((__interrupt__, auto_psv)) _T2Interrupt(void)
{
	_T2IF = 0;
#else
void pwmProcessISR(void)
{
#endif

	static UINT8 currentDuty = 0;

	currentDuty--;
	if (currentDuty == 0)
	{
		currentDuty = 100;
		pwmLightsOff();
	}

	/* we use greater than or equal since the
	 * value can be changed on the fly */
	if (dutyCycle1 >= currentDuty)
		pwmLightsOn1();

	if (dutyCycle2 >= currentDuty)
		pwmLightsOn2();

}

/* Use this function to override the dim value and turn it ON
 * By default the background light is on but dimmed. */
static void kpLights(UINT8 ledOnData)
{
	UINT8 newLedData;
	newLedData = ledOnData;
	/* only refresh LEDs on new data */
	if (kpLightOverrideOn != ledOnData)
	{
		kpLightOverrideOn = newLedData;
		/* in case pwm light was off, we turn it once on--like a refresh*/
	}
}

/* used to highlight a Key or not */
void kpHighlight(KEYP_ID keyNo, BOOL setON)
{
	if (setON == TRUE)
	{
		kpLights((kpLightOverrideOn | (1 << keyNo)));
	}
	else
	{
		kpLights((kpLightOverrideOn & ~(1 << keyNo)));
	}
}
/* This contains the value of the current duty value */
static UINT8 dimValueStored = FALSE;
/* This contains the value of the current duty value */
static UINT8 dimValueStoredMirror = 100;

/* Give a value between 0 and 100 where 100 is max */
void pwmSetLevel(UINT8 dimValueInput)
{
	/* Protection of the max. value. */
	if (dimValueInput > 100)
		dimValueInput = 100;

	// The keypad light is always more dimmed than the request */
	dutyCycle1 = dimValueInput / 2; // divide by 4

	/* Carry protection */
	if (dimValueInput < 5)
		dutyCycle2 = dimValueInput;
	else
		dutyCycle2 = dimValueInput - 5;

	dimValueStored = dimValueInput;
	//	dutyCycle3 = ((dimValueInput / 2)) + 50;
}

void pwmChangeLevel(void)
{
	if (dimValueStored > 10)
		dimValueStored -= 15;
	else
		dimValueStored = 100;

	pwmSetLevel(dimValueStored);
}

/* Returns TRUE if done, else false */
static BOOL pwmFadeOut(void)
{
	static UINT8 fadeOutValue = 0;

	if (fadeOutValue == 0)
	{
		fadeOutValue = dimValueStored;
		/* protection that the stored value never
		 * exceeds 100, dont sure if this is necc */
		if (dimValueStored < 100)
			dimValueStoredMirror = dimValueStored;
		else
			dimValueStoredMirror = 100;
	}

	fadeOutValue--;
	pwmSetLevel(fadeOutValue);

	if (fadeOutValue == 0)
	{
		kpLights(0);
		pwmLightsOff();

		/* Disable the keypad lights so we can restore the PWM*/
		kpLightOverrideOff = 0;
		// now all the LED's should be off so we restore the PWM
		// so that the POWER LED's is always on
		pwmSetLevel(100);

		return TRUE;
	}
	return FALSE;
}

/* Returns TRUE if done, else false */
static BOOL pwmFadeIn(void)
{
	static UINT8 fadeInValue = 0;

	fadeInValue += 2;
	pwmSetLevel(fadeInValue);

	/* If the current fade value is higher to the last value on PwrOff*/
	if (fadeInValue >= dimValueStoredMirror)
	{
		fadeInValue = 0;
		return TRUE;
	}
	return FALSE;
}

static BOOL outputsEnabled = FALSE;
void pollPwm(void)
{
	static UINT8 delayId = 0;
	static BOOL outputsEnabledMirror = FALSE;

	/* Only execute on a regular basis*/
	if (!(ucDelayMs(12, &delayId)))
		return;

	/* Something changed? */
	if (outputsEnabled != outputsEnabledMirror)
	{
		if (outputsEnabled == TRUE)
		{
			if (pwmFadeIn() == TRUE)
			{
				outputsEnabledMirror = outputsEnabled;
			}
			// Turn the keypads LEDs back online
			kpLightOverrideOff = 0xFF;
		}
		else
		{
			if (pwmFadeOut() == TRUE)
			{
				outputsEnabledMirror = outputsEnabled;
			}
		}
	}
}

void pwmEnableOutputs(BOOL setON)
{
	outputsEnabled = setON;

	/* To make sure that the outputs are
	 * off to make sure we start from off-> on*/
	 if (setON == TRUE)
	 {
	 pwmLightsOff();
	 pwmSetLevel(0);
	 }
}

void pwmInit(void)
{
	/* The following code example will enable Timer1 interrupts, load the Timer1
	 Period register and start Timer1.
	 When a Timer1 period match interrupt occurs, the interrupt service
	 routine must clear the Timer1 interrupt status flag in software.
	 */
	T2CON = 0x00; //Stops the Timer1 and reset control reg.
	TMR2 = 0x00; //Clear contents of the timer register

	// valid @32mhz Fosc
	PR2 = 1600; //Load the Period register with the value 0xFFFF
	_T2IP = 7; //Setup Timer1 interrupt for desired priority level
	// (This example assigns level 1 priority)
	IFS0bits.T2IF = 0; //Clear the Timer1 interrupt status flag
	IEC0bits.T2IE = 1; //Enable Timer1 interrupts
	T2CONbits.TON = 1; //Start Timer1 with prescaler settings at 1:1 and
	//clock source set to the internal instruction cycle
	//Output enable from serial drivers
	_TRISD11 = 0;
	// individual leds
	LATE &= ~(0x3F);

	/* We start up with PWM set to max. Every output should be initialized
	 * so nothing should light-up except the system-led */
	pwmSetLevel(100);
}

