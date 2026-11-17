# Data tables ported from the 2D game

| CSV | Row struct | Rows |
|---|---|---|
| DT_Skills.csv | FDFSkillRow | 33 |
| DT_Elements.csv | FDFElementRow | 14 |
| DT_WeaponClasses.csv | FDFWeaponClassRow | 16 |
| DT_Perks.csv | FDFPerkRow | 12 |
| DT_Passives.csv | FDFPassiveRow | 4 |
| DT_EnemyKinds.csv | FDFEnemyKindRow | 5 |
| DT_Bosses.csv | FDFBossRow | 5 |
| DT_SignatureParts.csv | FDFSignaturePartRow | 19 |
| DT_SignatureKeywords.csv | FDFSignatureKeywordRow | 40 |

Import: compile the C++ first, then drag each CSV into the Content Browser and pick the matching row struct.
Every table has Thai (TH) and English (EN) text. Files are UTF-8 with BOM.

Other files:
- `skill_vfx_briefs.csv` — what each skill should look like in 3D, its animation, and build priority (1 = first batch). Not a DataTable; a work list.
- `gacha_and_rules.json` — gacha costs, pity, refunds, weights, combo moves, signature-skill power formula.
- `all_game_data.json` — everything above in one file.
