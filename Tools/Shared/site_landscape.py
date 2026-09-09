"""One metric layout for authored service roads and coastal surface detail."""
import math
from earth_geography import height

# Authored service infrastructure around the existing simulator apron, not a survey.
ROAD=[(-137,-90),(-120,-73),(-120,74),(-103,91),(192,91),(215,68),(215,-68),(193,-90),(-137,-90)]

def smooth(a,b,x):
    t=max(0.,min(1.,(x-a)/(b-a)))
    return t*t*(3-2*t)

def dune_height(x,y):
    base=height(x,y)
    # Sub-metre procedural detail on the measured dune band. Sea and pad stay fixed.
    land=smooth(-3.2,-1.7,base)
    band=math.exp(-((x-(560-.045*y))/80)**2)*land*(1-smooth(2600,2900,abs(y)))
    detail=.5+.23*math.sin(x*.047+y*.022)+.16*math.sin(y*.075-x*.019)+.11*math.sin(x*.19+y*.043)
    return base+band*detail*.8

def road_height(x,y):
    # Existing zero-datum industrial apron and west access road.
    return .055
