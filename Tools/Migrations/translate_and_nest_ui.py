"""Checked migration of the player-facing interface to English."""
from pathlib import Path
R=Path(__file__).resolve().parents[1]/'Source/SuperHeavySim'
p=R/'Private/Recovery/RecoveryMenu.cpp'
s=p.read_text(encoding='utf-8')
translations={
'VOL EN PAUSE':'FLIGHT PAUSED','DU PAS DE TIR AUX BRAS DE LA TOUR.':'FROM LIFTOFF TO TOWER CAPTURE.',
'La simulation est suspendue.':'Your flight is paused.','Préparez votre prochain vol.':'Configure your mission.',
"Les choix de mission s'appliquent au prochain lancement.":'Mission changes apply to your next launch.',
'GRAPHISMES':'DISPLAY & GRAPHICS','Ajustez le rendu à votre écran et à votre machine.':'Configure display output and scene quality.',
'COMMANDES':'CAMERA & CONTROLS','Explorez le vol sous tous les angles.':'Move the mouse to look. Scroll to zoom. No mouse button required.',
'AFFICHAGE':'KEEP DISPLAY SETTINGS','Conserver cette résolution et ce mode d\'écran ?':'Keep this resolution and window mode?',
'ENREGISTRÉ':'SETTINGS SAVED','Vos paramètres graphiques sont appliqués et sauvegardés.':'Your graphics settings have been applied and saved.',
'CHOISIR UNE VUE':'CAMERA NETWORK','Cliquez pour rejoindre la caméra. V / Tab / Échap pour fermer. Molette pour ajuster le cadrage.':'Select a view. V / Tab / Esc to close. Move the mouse to orbit; scroll to zoom.',
'LUMIÈRE & CAMÉRA':'ENVIRONMENT & LENS','Composez votre lumière. Les changements sont visibles immédiatement.':'Set the light. Every change is visible in the scene.',
'Un vol suborbital, un retour autonome,\\nune capture au cœur de Starbase.':'A suborbital flight. An autonomous return.\\nA physical capture at Starbase.',
'LANCER LA SIMULATION   →':'PREPARE LAUNCH   →','REPRENDRE LE VOL   →':'RESUME FLIGHT   →',
'Choisir une caméra':'Camera network','Paramètres de simulation':'Mission setup','Paramètres graphiques':'Display & graphics','Lumière & caméra':'Environment & lens','Commandes & caméras':'Camera & controls',
'Relancer la simulation':'Restart flight','Retour à Starbase':'Return to Starbase','Quitter':'Exit simulator',
'Profil de mission':'Flight profile','Nominal · vent 4 m/s':'Nominal / 4 m/s wind','Vent de travers · 14 m/s':'Crosswind / 14 m/s wind','Masse à sec augmentée de 5 %':'Payload challenge / 5% more dry mass',
'Caméra au lancement':'Launch camera','Afficher la télémétrie de vol':'Show flight telemetry',
'Séparation → boostback → chute libre → landing burn → capture.\\nLes profils conservent le guidage autonome et la consommation de carburant.':'Separation → boostback → ballistic coast → landing burn → catch.\\nThe guidance system controls thrust, grid fins and reaction jets.',
"Résolution de l'écran":'Output resolution',"Mode d'affichage":'Window mode','Plein écran':'Fullscreen','Sans bordure · bureau':'Borderless / desktop','Fenêtré':'Windowed',
'Faible':'Low','Moyen':'Medium','Élevé':'High','Épique':'Epic','Personnalisé':'Custom','Préréglage global':'Quality preset','Échelle de rendu':'Scene render scale','100 % · natif':'100% / native',
'Ombres':'Shadows','Effets & atmosphère':'Effects & atmosphere','Anticrénelage':'Anti-aliasing','Éclairage global':'Global illumination','Reflets':'Reflections',
'Lumière des moteurs':'Engine light intensity','Désactivée':'Off','Discrète · 50 %':'Subtle / 50%','Standard · 100 %':'Standard / 100%','Intense · 150 %':'Bright / 150%','Très intense · 200 %':'Very bright / 200%',
"Limite d'images par seconde":'Frame rate limit','Illimitée':'Unlimited','Synchronisation verticale':'Vertical sync','APPLIQUER LES PARAMÈTRES':'APPLY SETTINGS',
'Pause / reprendre':'Pause / resume','Ouvrir le sélecteur de caméras':'Open camera network','Parcourir les 14 caméras':'Cycle through 14 cameras','Entrer / sortir de la caméra libre':'Toggle free flight camera',
'Afficher / masquer la télémétrie':'Show / hide telemetry','Relancer le vol':'Restart flight','Abandonner le vol, couper les moteurs':'Abort flight / shut down engines','Déplacer la caméra libre':'Move free flight camera','Clic droit maintenu':'Mouse movement','Orienter la caméra libre':'Look / orbit around focus','Monter / descendre':'Move up / down','Maj / molette':'Shift / scroll','Accélérer / ajuster la vitesse ou le zoom':'Boost movement / adjust speed or zoom','Échap':'Esc',
'Retour automatique dans %d secondes.':'Reverting automatically in %d seconds.','CONSERVER CET AFFICHAGE':'KEEP THESE SETTINGS',"Revenir à l'affichage précédent":'REVERT DISPLAY SETTINGS',
'00 / NUIT':'00 / NIGHT','06 / LEVER':'06 / DAWN','12 / MIDI':'12 / NOON','18 / COUCHER':'18 / DUSK',
'Brume côtière / brouillard volumétrique':'Coastal haze / volumetric fog','Flou de mouvement / obturateur':'Motion blur / shutter','Grain du capteur':'Film grain','Profondeur de champ · mise au point sur le véhicule':'Depth of field / vehicle focus',
'Heure solaire locale. Le vol reste suspendu lorsque vous réglez la lumière depuis le menu pause.':'Local solar time. Adjust the scene while the flight remains paused.',
'Retour aux graphismes':'Back to graphics','PAYSAGES & EXPLORATION':'LANDSCAPE & EXPLORATION','AU PLUS PRÈS DU VOL':'VEHICLE & TRACKING',
'Les vues spectateur conservent un champ large : le booster devient naturellement petit à plusieurs kilomètres.':'Observer cameras keep a wide field of view. Orbit views follow your mouse around their focus point.',
'REVENIR AU VOL':'RETURN TO FLIGHT','←  Retour':'←  Back',
}
for a,b in sorted(translations.items(),key=lambda kv:-len(kv[0])):s=s.replace(a,b)
# Semantic hierarchy keeps the existing audit page identifiers compatible.
s=s.replace('else if(Page!=0) ShowPage();','else if(Page!=0) ShowPage(ParentPage(Page));')
s=s.replace('if(NewPage==2 && Page!=2) ReadGraphics();','if(NewPage==2 && Page!=2) ReadGraphics();')
needle='    Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Text(Title,Page==0 && Home?64:34)];'
s=s.replace(needle,'''    if(Page==8) { Title=TEXT("SETTINGS");Caption=TEXT("Choose a system to configure."); }
    if(Page==9) { Title=TEXT("MISSION BRIEFING");Caption=TEXT("Understand the flight, then explore it from any camera."); }
    if(Page==10) { Title=TEXT("FLIGHT CONTROLS");Caption=TEXT("Take your time. Inspect the systems in motion."); }
    if(Page==11) { Title=TEXT("AUDIO");Caption=TEXT("Scene sound and launch acoustics."); }
    if(Page!=0) Rows->AddSlot().AutoHeight().Padding(0,0,0,14)[Text(ParentPage(Page)==8?TEXT("SETTINGS  /  SYSTEM"):TEXT("STARBASE  /  EXPERIENCE"),11,RecoveryUI::Muted)];
'''+needle)
s=s.replace('[PC](){ PC->LaunchFlight(); },true)', '[this](){ ShowPage(1); },true)',1)
start=s.index('        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Mission setup")')
end=s.index('        if(!Home)',start)
s=s[:start]+'''        if(!Home) Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Flight controls"),[this](){ ShowPage(10); })];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Settings"),[this](){ ShowPage(8); })];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Mission briefing"),[this](){ ShowPage(9); })];
'''+s[end:]
needle='    else if(Page==2)\n'
index=s.index(needle)
s=s[:index].replace('    }\n','    }\n',1)+s[index:]
# Add launch only on the setup page, at the end of its existing explanatory paragraph.
needle='    }\n    else if(Page==2)'
s=s.replace(needle,'''        Rows->AddSlot().AutoHeight().Padding(0,12,0,0)[Button(TEXT("INITIATE LAUNCH   →"),[PC](){ PC->LaunchFlight(); },true)];
    }
    else if(Page==2)''',1)
start=s.index('        const TArray<TPair<FString,FString>> Bindings=')
end=s.index('    }\n    else if(Page==4)',start)
s=s[:start]+'''        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("Mouse sensitivity"),15)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,22)[SNew(SSlider)
            .Value_Lambda([PC](){return (PC->MouseSensitivity-.1f)/1.9f;})
            .OnValueChanged_Lambda([PC](float V){PC->MouseSensitivity=.1f+V*1.9f;})
            .OnMouseCaptureEnd_Lambda([PC](){PC->SavePreferences();})];
        Toggle(Rows,TEXT("Automatic cinematic orbit until you move the mouse"),PC->bAutomaticOrbit,[PC](bool B){PC->bAutomaticOrbit=B;PC->SavePreferences();});
        const TArray<TPair<FString,FString>> Bindings={
            {TEXT("Mouse / scroll"),TEXT("Look or orbit / zoom or movement speed")},
            {TEXT("WASD / ZQSD"),TEXT("Move in free flight")},
            {TEXT("E / Ctrl / Shift"),TEXT("Up / down / boost movement")},
            {TEXT("V or Tab / C / F"),TEXT("Camera network / next view / free flight")},
            {TEXT("I / H"),TEXT("Learning overlay / telemetry")},
            {TEXT("[ / ]"),TEXT("Slower / faster playback (0.25–2×)")},
            {TEXT("Esc / R / X"),TEXT("Pause / restart / engine shutdown")}};
        for(const auto& Binding:Bindings)
            Rows->AddSlot().AutoHeight().Padding(0,0,0,14)[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(.4f)[Text(Binding.Key,14,RecoveryUI::Accent)]
                +SHorizontalBox::Slot().FillWidth(.6f)[Text(Binding.Value,14)]];
'''+s[end:]
start=s.index('    if(Page!=0 && Page!=4 && Page!=6)')
s=s[:start]+'''    else if(Page==8)
    {
        for(const auto& Item:TArray<TPair<FString,int32>>{{TEXT("Display & graphics"),2},{TEXT("Environment & lens"),7},{TEXT("Camera & controls"),3},{TEXT("Audio"),11},{TEXT("Mission setup"),1}})
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(Item.Key,[this,Id=Item.Value](){ShowPage(Id);})];
    }
    else if(Page==9)
    {
        const TArray<TPair<FString,FString>> Brief={
            {TEXT("01 / ASCENT & SEPARATION"),TEXT("33 engines lift the fully fuelled stack. Propellant use reduces its mass. The upper stage separates near the end of ascent.")},
            {TEXT("02 / BOOSTBACK & COAST"),TEXT("The boostback burn redirects the booster toward the launch site. Engines shut down during ballistic flight. Reaction jets control orientation in thin air.")},
            {TEXT("03 / ENTRY & LANDING"),TEXT("Grid fins become effective as air density rises. The landing burn slows descent and aligns the two catch fittings with the open tower arms.")},
            {TEXT("04 / PHYSICAL CAPTURE"),TEXT("The fittings contact the rails. Engines shut down and the rails carry the vehicle. A side impact blocks the body instead of moving it into place.")}};
        for(const auto& Item:Brief)
        { Rows->AddSlot().AutoHeight().Padding(0,0,0,7)[Text(Item.Key,15)];Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[Text(Item.Value,14,RecoveryUI::Muted)]; }
        Toggle(Rows,TEXT("Show live explanations during the flight (I)"),PC->bLearningOverlay,[PC](bool B){PC->bLearningOverlay=B;PC->SavePreferences();});
        Rows->AddSlot().AutoHeight()[Text(TEXT("Flight coefficients are estimates. The upper-stage flight after separation is illustrative."),12,RecoveryUI::Muted)];
    }
    else if(Page==10)
    {
        Choice(Rows,TEXT("Playback speed"),{TEXT("0.25× / inspect"),TEXT("0.5× / slow motion"),TEXT("1× / real time"),TEXT("2× / fast forward")},PC->PlaybackRate<.4?0:PC->PlaybackRate<.75?1:PC->PlaybackRate<1.5?2:3,
            [PC](int32 I){const float Rates[]={.25,.5,1,2};PC->SetPlaybackRate(Rates[I]);});
        Toggle(Rows,TEXT("Live explanations"),PC->bLearningOverlay,[PC](bool B){PC->bLearningOverlay=B;PC->SavePreferences();});
        Toggle(Rows,TEXT("Flight telemetry"),PC->bTelemetry,[PC](bool B){PC->SetTelemetry(B);});
        Rows->AddSlot().AutoHeight().Padding(0,12,0,10)[Button(TEXT("Camera network"),[this](){ShowPage(6);})];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Abort flight / shut down engines"),[PC](){if(auto* D=PC->GetDirector())D->AbortMission();PC->ResumeFlight();})];
    }
    else if(Page==11)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("Master volume"),16)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,25)[SNew(SSlider).Value_Lambda([PC](){return PC->MasterVolume;})
            .OnValueChanged_Lambda([PC](float V){PC->MasterVolume=V;})
            .OnMouseCaptureEnd_Lambda([PC](){PC->SavePreferences();})];
        Rows->AddSlot().AutoHeight()[Text(TEXT("Sound follows your camera distance, engine power and atmospheric density."),14,RecoveryUI::Muted)];
    }
'''+s[start:]
s=s.replace('[this](){ ShowPage(); })];','[this](){ ShowPage(ParentPage(Page)); })];')
p.write_text(s,encoding='utf-8')
p=R/'Private/Recovery/Interface/RecoveryMenu.h';s=p.read_text(encoding='utf-8')
s=s.replace('    void ReadGraphics();','''    static int32 ParentPage(int32 Id)
    {
        if(Id==2 || Id==3 || Id==7 || Id==11)return 8;
        if(Id==5)return 2;
        return 0;
    }
    void ReadGraphics();''');p.write_text(s,encoding='utf-8')
p=R/'Private/Recovery/SuperHeavyRecoveryHUD.cpp';s=p.read_text(encoding='utf-8')
for a,b in {'VITESSE':'SPEED','MASSE':'MASS','ERGOLS':'PROPELLANT','APPUIS PHYSIQUES':'RAIL CONTACTS','MOTEURS':'ENGINES','DÉCOLLAGE':'ASCENT','SÉPARATION':'SEPARATION','BALISTIQUE':'COAST','RENTRÉE':'ENTRY','FREINAGE':'LANDING BURN','APPROCHE':'APPROACH','MODÈLE DE VOL ESTIMÉ  ·  SIMULATION':'ESTIMATED FLIGHT MODEL  /  SIMULATION','V / TAB  VUES     F  LIBRE     H  HUD     ÉCHAP  PAUSE':'V  CAMERAS    F  FREE    I  LEARN    ESC  PAUSE','AXE %.2f M    CAP':'AXIS %.2f M    HEADING'}.items():s=s.replace(a,b)
needle='        return Layer+3;'
s=s.replace(needle,'''        if(PC && PC->bLearningOverlay)
        {
            static const TCHAR* Lessons[]={
                TEXT("Cryogenic propellant is loaded.\\nService vents release boil-off vapour.\\nThe guidance system is ready."),
                TEXT("Final countdown.\\nIgnition starts the engine sequence.\\nThe stack is released at liftoff."),
                TEXT("33 engines accelerate the stack.\\nBurning propellant lowers the mass.\\nThrust is limited to control acceleration."),
                TEXT("The stages separate.\\nThe booster prepares to turn back.\\nThe upper stage continues downrange."),
                TEXT("Boostback changes the flight path.\\nThe burn aims for the launch site.\\nReaction jets assist attitude control."),
                TEXT("Main engines are off.\\nGravity shapes the ballistic arc.\\nReaction jets work without ambient air."),
                TEXT("Atmospheric density increases.\\nGrid fins steer using aerodynamic force.\\nDrag removes part of the vehicle energy."),
                TEXT("The landing burn reduces descent speed.\\nThrust and attitude align the catch lugs.\\nThe arms remain solid obstacles."),
                TEXT("The booster descends onto both rails.\\nA valid first contact shuts engines down.\\nGravity transfers the load to the arms."),
                TEXT("Both fittings are supported by the rails.\\nThe engines and attitude jets are off.\\nContact friction holds the vehicle."),
                TEXT("Flight aborted. Engine thrust is zero.\\nUse Restart to prepare another attempt.")};
            Rect(28,112,405,180,FLinearColor(0,0,0,.72f));
            Text(D->GetPhaseLabel(),44,128,18,White,true);
            Text(Lessons[FMath::Clamp(int(D->Phase),0,10)],44,164,13,White);
            Text(FString::Printf(TEXT("PLAYBACK %.2f×   /   [ ] TO ADJUST"),PC->PlaybackRate),44,266,10,Grey);
        }
'''+needle)
p.write_text(s,encoding='utf-8')
print('ENGLISH_UI_HIERARCHY_READY')
