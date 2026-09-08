"""Render a contact sheet of the source props in Blender; does not edit sources."""
import bpy,sys,math
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
bpy.ops.wm.open_mainfile(filepath=str(ART_ROOT/'Starbase/Service/SiteServiceProps.blend'))
bpy.data.objects['SM_ServicePickup'].location.y=-2
bpy.data.objects['SM_TowableGenerator'].location.y=2
bpy.ops.object.camera_add(location=(11,-13,10));camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,.8))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=12
s=bpy.context.scene;s.camera=camera;s.render.engine='BLENDER_WORKBENCH'
s.display.shading.light='STUDIO';s.display.shading.studiolight_rotate_z=.6;s.display.shading.color_type='MATERIAL'
s.display.shading.show_shadows=True;s.display.shading.show_cavity=True;s.display.shading.cavity_type='BOTH'
s.display.shading.background_type='WORLD';s.world.color=(.06,.075,.09)
s.render.resolution_x=1600;s.render.resolution_y=1050;s.render.resolution_percentage=100
s.render.filepath=str(SAVED_ROOT/'service-props-source-review.png');bpy.ops.render.render(write_still=True)
