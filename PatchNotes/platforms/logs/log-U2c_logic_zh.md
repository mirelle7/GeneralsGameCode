# log-U2c_logic_zh

Group: 35 GeneralsMD game-logic files, ~110 conflict hunks. All markers removed (verified by grep).

| file | hunk | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|---|
| (31 files) AI, AIGroup, AIPlayer, AISkirmishPlayer, AIStates, Locomotor, ObjectCreationList, AIUpdate, JetAIUpdate, MissileAIUpdate, CleanupHazard, SupplyWarehouseDock, Float, NeutronMissile, ParticleUplink, PointDefenseLaser, SpectreGunship, DumbProjectile (5 of 6), Weapon (4 of 5), etc. | ~80 mechanical hunks | theirs (HEAD only differed by WWMath::Sinf/Cosf/Sqrtf vs upstream Sin/Cos/Sqrt names; normalised-equal, auto-applied) | upstream brief | no |
| AI.cpp | modifier = dist/attackPriorityDistanceModifier | theirs (Div_Safe); bobtista's `>0` guard dropped | duplicate guard, upstream wins | no |
| AISkirmishPlayer.cpp | angleOffset | theirs (Div_Safe) | same | no |
| BridgeBehavior, DumbProjectileBehavior, GenerateMinefield x2, MinefieldBehavior, SlowDeathBehavior, ActiveBody, DeliverPayloadAIUpdate, DozerAIUpdate, CommandButtonHuntUpdate, SlavedUpdate, ToppleUpdate, Weapon | trunk's ad-hoc div-by-zero guards | theirs (WWMath::Div_Safe) | upstream's wrapper replaces trunk's guards | note: fallback values follow upstream (e.g. Div_Safe default 0 where trunk used 1 / 999999) |
| PartitionManager.cpp | 8 hunks (RealOrDouble under RETAIL_COMPATIBLE_CRC, Sqrt/Pow rewiring) | theirs; trunk's float-only variant and WWMath::Sqrf dropped | upstream wins | no |
| RailroadGuideAIUpdate, POWTruckAIUpdate | Atan2 / Pow | theirs | | no |
| PhysicsUpdate.cpp | getForwardSpeed2D/3D | theirs (xezon dot-product + useLegacyForwardSpeed) | | no |
| ScriptEngine.cpp | xfer version | combined: upstream letterBox = version 6, trunk's m_objectCounts serialisation bumped to version 7 (currentVersion 7 non-retail; retail-compat still 5); header comment lists both | both sides keep function | YES: save-format version collision resolved by renumbering ours; check ScriptEngine.h has m_letterBoxActive and m_objectCounts |
| GameLogic.cpp | includes | combined: `Lib/Profile.h` (upstream) + our GgcRuntimeFlags.h and non-Win32 cstdio/cstdlib | rule 2 | no |

## Watch out for
- GameLogic.cpp still includes Common/Diagnostic/SimulationMathCrc.h and calls SimulationMathCrc::runBenchmark (non-conflicting, trunk's duplicate probe). If another group deletes SimulationMathCrc per the "one implementation" rule, drop that include/call, the RUN_MATH_BENCHMARK_REPLAY400_FLAG block and the comment near line 274.
- Leftover non-conflicting `WWMath::Sqrtf/Fabsf/Ceilf/Sinf/Cosf` calls exist in these files; upstream's wwmath.h declares all of them, so OK. WWMath::Sqrf is no longer used in my files.
- Files use LF/CRLF as they had; ScriptEngine.cpp and GameLogic.cpp edited preserving endings.
- ParticleUplinkCannonUpdate.cpp:554 still has raw sqrt(); identical in upstream, left as is.
