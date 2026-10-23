# Documentation content manifest

`manifest.json` is the complete content input for the documentation renderer. It lists all Markdown articles at its pinned public repository source revision plus every Markdown article/navigation document from the pinned upstream wiki. Source files remain complete; a title or excerpt is not a substitute.

## Renderer contract

Required top-level fields are `schemaVersion`, `sourceCommit`, `wikiSource`, `articles`, `missing` and `unavailable`. Each article includes `id`, `title`, `category`, `language`, root-relative `sourcePath`, stable `route`, original `sourceUrl` and `provenanceKind`. Added provenance fields are `originalSourcePath`, `sourceSha256`, `contentStatus` and `historicalPresentation`. `assets` inventories retained wiki images; `wikiAliases` resolves original wiki names, extensionless links, hyphenated names and upstream URLs into article IDs. The renderer must additionally resolve ordinary source-relative links and image paths from each article's own source directory, preserve anchors and handle inline HTML image tags safely.

Categories are explicit in the manifest. Languages are observed source labels (`en`, `zh-Hans`, `zh-Hant`, `mixed`), not invented translations. Wiki navigation documents remain full articles as well as navigation inputs. Local wiki assets live beneath `docs/wiki/upstream/images/` and retain exact source bytes. Do not fetch remote resources silently or claim that a static example controlled the installed application.

## Acceptance limits

A zero missing-file list means all files found in the two pinned source inventories were retained. It does not prove deployed routes, anchors, remote dependencies, image rendering, current screenshots, Cantonese translations, interactivity or companion runtime. Every unavailable category remains explicit. After a source change, refresh the source pin, per-file hashes and article inventory in the same reviewed change. Do not mark a required article complete merely because a renderer can open its specification draft.
