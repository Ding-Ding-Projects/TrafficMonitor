# Silent updater UTF-8 feed repair continuation

Continue the bounded repair from main fa9a645a72ba46e1f8744ddff7701e4bf180ea35. Public 1.0.23 RELEASES begins EF BB BF; the old parser counted those bytes in its SHA-1 field. The repair verifies raw feed length/SHA-256, then skips only a leading UTF-8 marker for parsing, retaining every package digest/size/basename/entry-count and accepted-main check. Fixed validation substages preserve numeric error 13.

Production is pending. No tests, reviews, audits, runtime probes, interface interaction or screenshots run. No installation mutation, request trigger or preference change occurs here. Parent owns push, main integration, publication and accepted installation. Automatic-update acceptance and full Stage 2 remain unfinished. Matching guide/wiki section is prepared; wiki publication remains pending.
