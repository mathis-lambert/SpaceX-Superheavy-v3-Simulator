"""Canonical source, output, and content roots for standalone authoring tools."""
from pathlib import Path
PROJECT_ROOT=Path(__file__).resolve().parents[2]
ART_ROOT=PROJECT_ROOT.parent/'ArtSource'
CONTENT_ROOT='/Game/Starbase'
SAVED_ROOT=PROJECT_ROOT/'Saved'/'Recovery'
