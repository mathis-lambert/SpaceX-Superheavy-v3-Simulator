"""Bounded directional water spectrum shared by every geographic surface.

Units are metres/seconds. Deep-water dispersion: omega = sqrt(g*k).
Long swells deform geometry; short wind waves affect shading only. Pixel
derivatives suppress unresolved slopes rather than producing distant moire.
Reference: NVIDIA GPU Gems, Effective Water Simulation from Physical Models.
"""
import math

# wavelength, amplitude, heading degrees, phase radians. Non-harmonic periods
# and a wind-biased spread avoid the old crossed, repeating normal-map grid.
SWELLS=((61.7,.23,24,.6),(37.3,.13,-13,2.1))
WIND_WAVES=((19.1,.105,16,4.4),(11.3,.076,37,1.2),
            (6.7,.048,-7,5.8),(3.9,.032,29,2.7),
            (2.17,.022,4,.3),(1.23,.016,42,4.1),
            (.71,.010,-18,1.7),(.39,.006,21,3.4),
            (.21,.0038,9,5.1),(.113,.0021,35,2.3))


def wave_code(waves, slopes=False):
    lines=['float2 xy=P.xy*.01;','float2 slope=0;' if slopes else 'float height=0;']
    for i,(wavelength,amplitude,heading,phase) in enumerate(waves):
        k=math.tau/wavelength
        x,y=math.cos(math.radians(heading)),math.sin(math.radians(heading))
        omega=math.sqrt(9.80665*k)
        lines.append(f'float phase{i}=dot(xy,float2({k*x:.12f},{k*y:.12f}))-T*{omega:.12f}+{phase};')
        if slopes:
            # Fade before phase changes by a pixel; normal detail then tends to
            # its unresolved roughness instead of scintillating on the horizon.
            lines.append(f'slope+=float2({k*x*amplitude:.12f},{k*y*amplitude:.12f})*cos(phase{i})*(1-smoothstep(.8,2.8,fwidth(phase{i})));')
        else:
            lines.append(f'height+={amplitude}*sin(phase{i});')
    return '\n'.join(lines)


def normal_code():
    return wave_code(SWELLS+WIND_WAVES,slopes=True)+'''
float3 up=normalize(P+float3(0,0,637100000));
float3 east=normalize(cross(float3(0,1,0),up)),north=cross(up,east);
float viewDistance=length(Camera-P);
slope*=1-smoothstep(120000,650000,viewDistance);
float near=1-smoothstep(15000,150000,viewDistance);
float ripplePhase=P.x*.23+P.y*.11+sin(P.y*.017)*.8;
float ripples=sin(ripplePhase)*.035*(1-smoothstep(.5,2,fwidth(ripplePhase)))*(1-smoothstep(1200,6500,viewDistance));
float3 land=normalize(Terrain+(east*Ground.x+north*Ground.y)*near*.55+(east*.9+north*.43)*ripples);
return normalize(lerp(land,normalize(up-east*slope.x-north*slope.y),W));'''


def displacement_code():
    return wave_code(SWELLS)+'''
float shoreDistance=(H.g-.5)*256;
float shoal=lerp(1,smoothstep(0,35,shoreDistance),Coverage);
float fade=1-smoothstep(150000,400000,length(P-Camera));
return normalize(P+float3(0,0,637100000))*height*100*H.r*Coverage*shoal*fade;'''
