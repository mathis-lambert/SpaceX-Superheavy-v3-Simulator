#pragma once

#include "CoreMinimal.h"
#include "SuperHeavyPidController.generated.h"

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyPidController
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	double Kp = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	double Ki = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	double Kd = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	double IntegralLimit = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bClampOutput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (EditCondition = "bClampOutput"))
	double OutputMin = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (EditCondition = "bClampOutput"))
	double OutputMax = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "PID")
	double Integral = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "PID")
	double PreviousError = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "PID")
	bool bHasPreviousError = false;

	double Update(double Error, double DeltaTime)
	{
		if (DeltaTime <= UE_SMALL_NUMBER)
		{
			return ClampOutput(Kp * Error);
		}

		Integral += Error * DeltaTime;
		if (IntegralLimit > 0.0)
		{
			Integral = FMath::Clamp(Integral, -IntegralLimit, IntegralLimit);
		}

		const double Derivative = bHasPreviousError ? (Error - PreviousError) / DeltaTime : 0.0;
		PreviousError = Error;
		bHasPreviousError = true;

		return ClampOutput((Kp * Error) + (Ki * Integral) + (Kd * Derivative));
	}

	double UpdateWithMeasuredRate(double Error, double MeasuredRate, double DeltaTime)
	{
		if (DeltaTime > UE_SMALL_NUMBER)
		{
			Integral += Error * DeltaTime;
			if (IntegralLimit > 0.0)
			{
				Integral = FMath::Clamp(Integral, -IntegralLimit, IntegralLimit);
			}
		}

		PreviousError = Error;
		bHasPreviousError = true;

		return ClampOutput((Kp * Error) + (Ki * Integral) - (Kd * MeasuredRate));
	}

	void Reset()
	{
		Integral = 0.0;
		PreviousError = 0.0;
		bHasPreviousError = false;
	}

private:
	double ClampOutput(double Output) const
	{
		return bClampOutput ? FMath::Clamp(Output, OutputMin, OutputMax) : Output;
	}
};
