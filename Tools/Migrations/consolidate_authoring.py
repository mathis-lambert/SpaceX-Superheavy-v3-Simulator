"""Replace duplicated material graph primitives with one checked implementation."""
from pathlib import Path
import ast
P=Path(__file__).resolve().parents[2];editor=P/'Tools/Editor'
groups={
 'build_overhaul_presentation.py':({'prop','ex','scalar','const','custom','connect','sample','save'},'prop, expression as ex, scalar, constant as const, custom, connect, sample, save'),
 'build_earth_environment.py':({'prop','ex','constant','custom','sample','connect','save'},'prop, expression as ex, constant, custom, sample, connect, save'),
 'build_flight_presentation.py':({'expr','scalar','color','constant','custom','sample','save','connect'},'expression as expr, scalar, color, constant, custom, sample, save, connect'),
 'build_recovery_vfx.py':({'expr','scalar','custom','save','connect'},'expression as expr, scalar, custom, save, connect')}
for name,(functions,imports) in groups.items():
    path=editor/name;text=path.read_text(encoding='utf-8');tree=ast.parse(text);lines=text.splitlines(keepends=True)
    remove=set()
    for node in tree.body:
        if isinstance(node,ast.FunctionDef) and node.name in functions:remove.update(range(node.lineno-1,node.end_lineno))
    text=''.join(line for index,line in enumerate(lines) if index not in remove)
    # Old builders used the reversed (expression, material, property) convention.
    if name in ['build_flight_presentation.py','build_recovery_vfx.py']:
        lines=text.splitlines(keepends=True);changes=[]
        for node in ast.walk(ast.parse(text)):
            if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='connect':
                node.args[0],node.args[1]=node.args[1],node.args[0]
                start=sum(len(x.encode('utf-8')) for x in lines[:node.lineno-1])+node.col_offset
                end=sum(len(x.encode('utf-8')) for x in lines[:node.end_lineno-1])+node.end_col_offset
                changes.append((start,end,ast.unparse(node).encode('utf-8')))
        raw=text.encode('utf-8')
        for start,end,value in sorted(changes,reverse=True):raw=raw[:start]+value+raw[end:]
        text=raw.decode('utf-8')
    text=text.replace('from project_paths import PROJECT_ROOT, ART_ROOT','from project_paths import PROJECT_ROOT, ART_ROOT\nfrom unreal_materials import '+imports)
    path.write_text(text,encoding='utf-8')
for p in (P/'Tools').rglob('*.py'):
    if 'Migrations' in p.parts:continue
    text=p.read_text(encoding='utf-8')
    text=text.replace("/'Scripts'/'audit_", "/'Tools'/'Tests'/'audit_")
    text=text.replace("/'Scripts'/s", "/'Tools'/'Tests'/s")
    text=text.replace("'Scripts','build_recovery_vfx.py'","'Tools','Editor','build_recovery_vfx.py'")
    p.write_text(text,encoding='utf-8')
print('AUTHORING_HELPERS_CENTRALIZED')
