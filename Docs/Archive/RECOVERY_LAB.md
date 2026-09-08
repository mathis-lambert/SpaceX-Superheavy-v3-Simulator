# Super Heavy — Recovery Lab

## Lancer et piloter les vues

Ouvrir le projet avec Unreal 5.8.2 puis Play, ou lancer `Scripts/launch_recovery.ps1`. L'accueil présente Starbase en 3D et attend le bouton **Lancer la simulation**. Les paramètres permettent de choisir le scénario, la caméra initiale et la télémétrie. **Échap** suspend réellement le monde, puis ouvre la reprise, les réglages, le redémarrage et le retour à l'accueil. En Play dans l'éditeur, Échap reste un raccourci de l'éditeur pour arrêter PIE ; utiliser le lancement autonome pour tester le menu pause au clavier.

Les réglages graphiques incluent résolution, mode d'écran, qualité globale et par catégorie, échelle de rendu, limite d'images par seconde, VSync et intensité des lumières des moteurs. **Appliquer** les sauvegarde. Une résolution ou un mode d'écran non confirmé revient à sa valeur précédente après 15 secondes, y compris en pause. Les réglages de scénario et de caméra initiale s'appliquent au prochain lancement ; la télémétrie est immédiate.

| Touche | Action |
|---|---|
| Échap | Pause, paramètres, reprise ; retour depuis un sous-menu |
| Espace | Lancer la mission prête |
| R | Réinitialiser puis relancer le scénario |
| V / Tab | Sélecteur direct de 14 caméras, en vol |
| C | Parcourir les quatorze caméras |
| F | Entrer en caméra libre / revenir à la vue précédente |
| H | Masquer ou afficher la télémétrie |
| X | Abandonner et couper les moteurs |
| F1 / F2 / F3 | Préparer Nominal / Crosswind / Offset |
| Molette | Distance des vues suivies ; vitesse de déplacement en caméra libre |
| ZQSD ou WASD | Déplacer la caméra libre |
| Bouton droit + souris | Orienter la caméra libre |
| E / Ctrl gauche | Monter / descendre en caméra libre |
| Maj gauche | Déplacement libre dix fois plus rapide |

Les vues sont : suivi, optique du pas de tir, moteurs, optique de la tour, grid fins, caméra embarquée vers le bas, poursuite de trajectoire, orbite cinématique, caméra libre, spectateur à 3 km, spectateur à 8 km, panorama de Starbase, horizon terrestre et Terre entière. Les transitions aboutissent en 1,2 seconde. La composition de suivi évolue progressivement lors de la séparation. Le suivi et les visuels utilisent la pose physique de fin de frame, ce qui supprime le retard à grande vitesse et le glissement de l'étage supérieur.

## Vol autonome

La mission monte à environ 97 km et parcourt environ 76 km horizontalement. Elle comprend 33 moteurs en montée, séparation et rotation, boostback sur 13 moteurs, plus de deux minutes sans moteurs, orientation aérodynamique par trois grid fins, landing burn de 13 à 3 moteurs, alignement des deux ferrures et capture.

Le booster suit les forces intégrées par Chaos. Il ne reçoit pas de positions imposées en vol. Sa masse diminue avec la consommation : 5 617 t pour l'ensemble initial chargé, dont 1 750 t d'étage supérieur estimé ; environ 295 t de booster à l'arrivée dans le scénario nominal actuellement testé. La séparation retire l'étage supérieur, sans réinitialiser les ergols.

Les grid fins perdent leur autorité avec la pression dynamique ; un système de réaction consommant son propre stock agit dans l'air raréfié. Les moteurs sont effectivement coupés pendant le vol balistique et la rentrée. Les hypothèses et les sources publiques sont détaillées dans [FLIGHT_MODEL.md](FLIGHT_MODEL.md).

## Capture et configuration

| Asset | Rôle |
|---|---|
| `/Game/Recovery/Maps/L_RecoveryLab` | Scène principale |
| `/Game/Recovery/Blueprints/BP_LaunchTower` | Tour réutilisable |
| `/Game/Recovery/Blueprints/BP_RecoveryDirector` | Gestion de mission |
| `/Game/Recovery/Data/DA_RecoveryMission` | Masses, poussée, trajectoire, aérodynamique, vent et tolérances |
| `Source/SuperHeavySim/{Public,Private}/Recovery` | Code natif |

Les deux ferrures importées sont à `(±4,99 ; 0,0079 ; 62,7978)` m depuis la base. Le cap de capture est de 90° par rapport à la tour ; le centre des bras est à `61,5678` m au-dessus de la base cible. Les deux boîtes de collision de ferrure sont soudées au corps rigide ; les bras possèdent des rails et des volumes structurels bloquants. Le guidage acquiert l'ouverture par l'avant en gardant une garde au-dessus de la tour, puis compense l'inclinaison due au vent pour positionner les ferrures.

Le premier contact de ferrure validé coupe les moteurs si l'erreur des ferrures est inférieure à 50 cm, la vitesse à 1,5 m/s et l'inclinaison à 3°. La gravité transfère le poids sur les rails, sans maintien de pose. Deux appuis stables pendant 0,6 seconde permettent de déclarer la capture ; huit secondes supplémentaires vérifient son maintien. Le composant de contrainte historique reste présent pour la compatibilité du Blueprint, mais n'est connecté à aucun corps. La déformation et les contacts mécaniques détaillés des bras restent une approximation.

Le profil est copié au lancement du scénario. Les variantes ajoutent un vent de 14 m/s selon Y, ou une masse sèche augmentée de 5 % (nom historique `Offset`). La tour doit garder une échelle de 1 et des rails verticaux. Lancement et capture partagent l'axe à 24 m devant la tour ; le lancement commence à 12 m de hauteur de base, la capture à 20 m.

Le modèle fourni utilise une capsule `COL_Body_Main`, l'échelle `(22.5,22.5,80)`, un décalage de base de 35,44 m et un bras de levier moteur de 31 m. Un autre véhicule exige une recalibration de cet adaptateur.

## Vérifier et reconstruire

Depuis le dossier du projet :

```powershell
./Scripts/test_recovery.ps1
./Scripts/test_recovery.ps1 -Scenarios Nominal,Crosswind,Offset -FrameRates 30,15
```

Les tests contrôlent les phases, l'altitude, la consommation, l'absence de poussée en chute libre, l'action des grid fins, l'alignement des ferrures et la tenue de la capture. Chaque exécution archive les JSON, CSV et logs dans `Saved/Recovery/Tests/<date>`. Le code de sortie du lanceur Unreal ne suffit pas à prouver la réussite.

Les scripts Blender `build_tower_meshes.py` et `build_flight_art.py` produisent les FBX et les sources `../assets/recovery/`. `bake_vapor_atlas.py` génère l'atlas de fumée avec NumPy et Pillow. Les scripts Python Unreal s'exécutent via le commandlet PythonScript. Ordre de reconstruction : `build_recovery_scene.py`, éventuellement `migrate_flight_profile_v2.py` pour rétablir les paramètres nominaux, puis **`build_flight_presentation.py`**, **`build_earth_environment.py`** et `audit_recovery_assets.py`. Préparer les sources géographiques selon [RECOVERY_EARTH.md](RECOVERY_EARTH.md) avant la passe Terre. Fermer l'éditeur et le jeu avant d'écrire les assets. La traînée GPU et ses réglages sont décrits dans [RECOVERY_VFX.md](RECOVERY_VFX.md).

La reconstruction de scène remplace les acteurs préfixés `Recovery_` de cette carte générée. Dupliquer la carte avant de créer un site personnalisé. Les assets originaux du booster et les anciens contrôleurs restent disponibles.

## Portée

Ce simulateur est un modèle suborbital estimé, pas une télémétrie ni un logiciel validé par SpaceX. Les masses sèches, Isp, coefficients aérodynamiques, lois de commande et marges non publiés sont explicitement des hypothèses. L'étage supérieur est un proxy visuel après séparation. La Terre utilise des images NASA, et Boca Chica des orthophotos NAIP et du relief 3DEP nettoyé. Les installations du site restent une composition adaptée au simulateur ; les panaches sont des effets visuels. Le dégagement nécessaire à l'acquisition du couloir de capture prolonge le landing burn par rapport à certaines chronologies publiques.

Après les scripts de présentation historiques et **`build_earth_environment.py`**, appliquer la refonte selon [RECOVERY_OVERHAUL.md](RECOVERY_OVERHAUL.md). Les scripts historiques seuls rétablissent des matériaux et réglages antérieurs. Voir [RECOVERY_EARTH.md](RECOVERY_EARTH.md) pour les données géographiques et leurs limites.


Pour forcer une fenêtre en 1440p : `./Scripts/launch_recovery.ps1 -Width 2560 -Height 1440`. Ajouter `-Fullscreen` pour le plein écran. Sans arguments, le lanceur respecte les réglages enregistrés. Les nouvelles installations démarrent en 1080p, rendu natif, qualité épique et limite de 60 images par seconde.

`-RecoveryNoMenu` conserve le lancement direct pour les outils. Les tests `-RecoveryAutoExit` et les captures `-RecoveryReview` contournent également l'accueil. L'audit rendu `-RecoveryUIAudit` parcourt les menus, lance, mesure le gel de la physique et de l'horloge pendant une pause, vérifie le retour automatique de résolution, reprend puis revient à l'accueil. Il écrit ses contrôles et captures dans `Saved/Recovery/InterfaceAudit` avant de quitter.

**Lumière & caméra**, à l'accueil comme en pause, propose le curseur d'heure solaire 0–24 h, la quantité de brouillard, le flou de mouvement, le grain et la profondeur de champ. Les réglages agissent immédiatement et sont sauvegardés à la fin du glissement. La pause conserve le vol immobile pendant les changements de lumière. Le HUD est dessiné par Slate à la résolution de sortie après les effets caméra ; il ne reçoit ni grain ni flou de scène.
