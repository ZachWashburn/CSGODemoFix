# CSGODemoFix
A Fix For CS:GOs Broken Demos

Counter-Strike's Demos have been broken since ~2017. This is due to string tables (specifically the GameRulesCreation table) being both not sent by the server, and incorrectly written to the dem_stringtables demo message. This causes a full game crash as the function Q_stricmp() recieves a nullptr. Ignoring this condition, leads to a crash in RecvTables_Guts().

This reposisitory offers a solution. Simply inject the dll into the game process, attempt loading of the demo 3 times, and the demo will load correctly. Occasionally localization strings for skins will cause an engine error, this is unavoidable in cases the server never sent the client the correct stringtables when the demo_restart network message was sent from the client. 

We patch it up by parsing the string table ourselves, and rewriting it. taking care to omit invalid fields and attempt to guesstimate correct values we are missing. 3 loads are needed for, #1 function hooking, #2 parse table correctly when it triggers and exception, #3 load with our fix 
