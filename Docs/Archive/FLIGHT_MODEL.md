# Vol suborbital — modèle et provenance

Le simulateur utilise le maillage de booster fourni, un Blueprint de tour réutilisable et un Data Asset de mission. Chaos intègre les forces et les moments. La trajectoire du booster n'est ni une animation ni une suite de positions imposées. L'étage supérieur ajouté au rendu est une silhouette de contexte : sa masse est transportée jusqu'à la séparation, mais son vol ultérieur est seulement illustré.

## Données publiques retenues

| Donnée | Valeur de référence | Source |
|---|---:|---|
| Moteurs du booster | 33, dont 13 orientables | [SpaceX — Starship](https://new.spacex.com/vehicles/starship) |
| Capacité d'ergols du Super Heavy V3 | 3 650 t | Même source |
| Diamètre / hauteur du V3 publié | 9 m / 72 m | Même source |
| Poussée d'un Raptor 3 au niveau de la mer | 250 tonnes-force, soit 2 451 662,5 N | [SpaceX — Introducing Starship V3, 12 mai 2026](https://new.spacex.com/updates) |
| Grid fins du V3 | 3 ; points de capture revus avec cette génération | Même source |

La [chronologie publique du vol 12](https://www.spacex.com/launches/starship-flight-12?channel=MSN) prévoit MECO à 2:22, boostback de 2:30 à 3:30 et landing burn de 6:34 à 6:59. Le vol réel n'a pas réalisé un retour nominal : son boostback a été partiel et il s'est terminé par un impact en mer. Notre scénario est donc une récupération nominale estimée, pas une reproduction certifiée de ce vol ni une télémétrie SpaceX.

## Paramètres estimés et configurables

La masse sèche (210 t), la masse de l'étage supérieur (1 750 t), les Isp 327–350 s, le plancher de modulation, les réserves et le contrôle de réaction sont des hypothèses de simulation. Les coefficients de traînée et de portance, les performances des actionneurs et la correction de dérive d'atterrissage sont aussi des estimations. Le Data Asset les regroupe explicitement dans des catégories « Estimated ».

La masse initiale inclut le booster, les ergols, l'étage supérieur et le stock de contrôle de réaction. La consommation suit `dm = poussée / (Isp × g0) × dt`, selon la [définition NASA de l'impulsion spécifique](https://www.grc.nasa.gov/www/BGH/specimp.html). La séparation retire uniquement la masse de l'étage supérieur ; les ergols restants ne sont jamais réinitialisés en vol. L'inertie varie avec la masse, en conservant une répartition simplifiée et le même centre de masse.

L'atmosphère suit les sept couches de l'[US Standard Atmosphere 1976](https://ntrs.nasa.gov/api/citations/20050207438/downloads/20050207438.pdf), jusqu'à environ 86 km géométriques. Au-dessus, une continuation exponentielle remplace le modèle thermochimique complet. La température de surface et le vent sont configurables. La gravité diminue avec l'altitude. La pression dynamique vaut `q = ρ Vair² / 2` ; les forces de chaque grid fin et leurs limites de commande en dépendent. Le contrôle de réaction prend le relais dans l'air raréfié et consomme son propre stock.

## Navigation et capture

La mission comprend montée sur 33 moteurs, séparation et rotation, boostback sur 13 moteurs, côte balistique, descente atmosphérique sans moteurs, landing burn et capture. Une prédiction de retour est recalculée depuis la position et la vitesse mesurées. Le passage au freinage combine altitude, énergie verticale et capacité de poussée. Le contrôleur résout le couplage entre poussée latérale et effort normal du corps. Un dégagement au-dessus de la tour est conservé tant que le couloir terminal n'est pas acquis ; cette correction peut prolonger le burn par rapport aux vols publics.

Les ferrures ont été mesurées dans le Blender fourni puis converties dans le repère effectivement importé dans Unreal : environ `(±4,99 ; 0,0079 ; 62,7978)` mètres depuis la base. Le cap de capture est 90° par rapport à la tour. La troisième grid fin, nommée historiquement `GF_YM`, est en réalité sur +Y dans Unreal après conversion des axes.

Le lancement et la capture partagent maintenant le même axe, à 24 m devant la tour. Le guidage acquiert l'ouverture depuis l'avant, en restant au-dessus de la tour jusqu'à l'alignement. Deux volumes de ferrure font partie du corps rigide Chaos du booster ; les rails et les structures des bras bloquent réellement ce corps. Une première prise d'appui lente et correctement placée coupe les moteurs et le contrôle d'attitude : la gravité pose l'autre ferrure sur son rail. La capture est reconnue après stabilisation sur les deux appuis, puis vérifiée pendant huit secondes. Aucune contrainte ne fixe la pose, aucune vitesse n'est remise à zéro lors du contact. La compliance et la déformation des bras ne sont pas modélisées ; leurs volumes de collision et le frottement restent simplifiés.

Le contrôle de réaction applique six forces aux emplacements des modules modélisés. Elles forment des couples de force, sans translation parasite, et consomment un stock distinct selon leur poussée totale et l'Isp estimée. Positions, dimensions, poussée et Isp des buses sont des hypothèses, faute de données publiques détaillées.

## Rendu et limites

La Terre complète emploie les images NASA ; le littoral utilise des orthophotos NAIP et le relief USGS 3DEP, avec les limites décrites dans RECOVERY_EARTH.md. Les installations sont une composition adaptée au simulateur. Les panaches suivent les tuyères et s'élargissent avec la baisse de pression ; il s'agit d'effets visuels, pas de CFD de combustion. Le maillage utilisateur reste conservé, y compris ses différences avec le V3 publié. Le nouveau Starship comporte quatre volets, des joints et six tuyères ; sa trajectoire après séparation reste une représentation visuelle.

Ce travail améliore les mécanismes physiques et leur transparence. Les données d'essais publics ne suffisent pas à identifier les coefficients aérodynamiques, les lois de commande, les masses et les marges internes de SpaceX. Les journaux de test valident le fonctionnement de ce modèle, pas sa fidélité d'ingénierie à un véhicule réel.
