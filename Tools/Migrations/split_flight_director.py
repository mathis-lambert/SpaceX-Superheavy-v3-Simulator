"""Separate mission lifecycle, guidance forces and telemetry without changing behavior."""
from pathlib import Path
P=Path(__file__).resolve().parents[2]
folder=P/'Source/SuperHeavySim/Private/Recovery/Flight'
f=folder/'SuperHeavyRecoveryDirector.cpp';text=f.read_text(encoding='utf-8')
includes=text[:text.index('ASuperHeavyRecoveryDirector::ASuperHeavyRecoveryDirector()')]
a=text.index('void ASuperHeavyRecoveryDirector::UpdateMass()')
b=text.index('FVector ASuperHeavyRecoveryDirector::AttitudeTorque(')
c=text.index('void ASuperHeavyRecoveryDirector::WriteResult(')
d=text.index('void ASuperHeavyRecoveryDirector::EndPlay(')
(folder/'RecoveryGuidance.cpp').write_text(includes+text[b:c],encoding='utf-8')
(folder/'RecoveryTelemetry.cpp').write_text(includes+text[a:b]+text[c:d],encoding='utf-8')
f.write_text(text[:a]+text[d:],encoding='utf-8')
print('FLIGHT_DIRECTOR_SPLIT')
