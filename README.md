# Jam-for-the-Epic-Games--STOLEN-OATH

## Reglas principales: 
-Una persona responsable por cada tarea.
-Toda modificación importante debe quedar registrada.
-No trabajar directamente sobre la misma rama de Git.
-Hacer commits frecuentes y descriptivos.
-No integrar assets sin verificar su licencia.
-Las decisiones de diseño deben documentarse.
-Realizar una build jugable cuanto antes.
-Reservar tiempo para testing y entrega.

## Como poner assets para la IA
*Para todos los artistas del projecto*
Guardar assets que no fueron importados al projecto de Unreal dentro de la carpeta "NonUnrealAssetDump"

## English reminder: Unreal Blueprint tuning values

After a successful C++ build, open the relevant Blueprint, click **Class Defaults**, then search for the property names below. These are inherited C++ properties; they are not nodes in the Event Graph.

### Existing player Blueprint: `BP_ThirdPersonCharacter`

This Blueprint inherits from `AStolenOathCharacter`. Search for:

- `MaxHealth`
- `BasicAttackAnimation`, `BasicAttackAnimationPlayRate`, `BasicAttackTraceHeight`, `BasicAttackRange`, `BasicAttackRadius`, `BasicAttackImpulse`, `BasicAttackDamage`
- `AttackResonanceGain`, `MaxResonanceDamageBonus`
- `DodgeDuration`, `Dodge Distance` (C++ property: `DodgeSpeed`), `DodgeAnimationPlayRate`
- `NormalDodgeResonance`, `PreciseDodgeResonance`, `PerfectDodgeResonance`, `PreciseDodgeWindow`, `PerfectDodgeWindow`
- `DirectorCutDamage`, `DirectorCutRange`, `DirectorCutStunDuration`, `DirectorCutAnimationPlayRate`
- `RelicInteractionRange`, `TrainingRelicSpawnDistance`, `TrainingSentinelSpawnDistance`, `TrainingSentinelGroundOffset`
- `bSpawnTrainingSentinel`, `TrainingRelicClass`, `TrainingSentinelClass`

The training sentinel and relic are spawned as native C++ classes by default. To edit their class-specific settings in Blueprint defaults, create a child Blueprint for each class, then assign it to `TrainingSentinelClass` or `TrainingRelicClass` on `BP_ThirdPersonCharacter`.

### Sentinel Blueprint: child of `AStolenOathTrainingSentinel`

No sentinel child Blueprint is currently in the project. After creating one and assigning it to `TrainingSentinelClass`, its Class Defaults include:

- `MaxHealth`, `AttackRange`, `AttackInterval`, `TelegraphDuration`, `AttackDamage`
- `FollowSpeed`, `FollowStopDistance`, `TurnSpeed`
- `PhaseTwoAttackInterval`, `PhaseTwoTelegraphDuration`, `PhaseTwoDamageMultiplier`, `PhaseTwoHealthFraction`, `PhaseTwoEntryDelay`
- `OverheadAttackRadius`, `SweepAttackInnerRadius`, `SweepAttackRange`, `SweepAttackDotThreshold`
- `InitialAttackDelay`, `DefeatRespawnDelay`, `HitFlashDuration`
- `BaseMeshScale`, `SentinelBaseMaterial`, `PhaseOneColor`, `PhaseTwoColor`, `HitFlashColor`
- `WarningMaterial`, `OverheadWarningColor`, `SweepWarningColor`, `WarningArcSegments`, `WarningRadialSubdivisions`, `WarningEdgeWidth`, `WarningGroundOffset`

### Relic Blueprint: child of `AStolenOathTrainingRelic`

No relic child Blueprint is currently in the project. After creating one and assigning it to `TrainingRelicClass`, search for `RelicBodyScale`, `RelicCoreScale`, `RelicCoreRotation`, and `RelicColor`.

### HUD Blueprint: child of `AStolenOathHUD`

No HUD child Blueprint is currently in the project. After creating one, assign it as the HUD Class in `BP_ThirdPersonGameMode`. Its Class Defaults include:

- Layout: `HUDLeft`, `HUDTop`, `ResourceBarWidth`, `ResourceBarHeight`, `RelicPromptWidth`, `RelicPromptHeight`, `RelicPromptVerticalAnchor`, `BossBarWidth`, `BossBarTop`, `BossBarHeight`, `CombatCueMaxWidth`, `CombatCueHeight`, `CombatCueVerticalAnchor`
- Colors: `ResourceBarBackgroundColor`, `HealthBarColor`, `ResonanceBarColor`, `BossHealthBarColor`, `CombatCuePanelColor`
