"""Rebuild only the RCS optical material."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from attitude_gas import build_attitude_gas

build_attitude_gas()
print('ATTITUDE_GAS_READY')
