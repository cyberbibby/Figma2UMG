---
name: figma-umg-structure-audit
description: Audit Figma layer hierarchy for Figma2UMG before prefix naming. Use when UMG import needs hierarchy changes such as moving, flattening, wrapping, splitting layers, setting BTN_ hit areas, IMG_/Image_ sources, WBP_ boundaries, or LST_/TLV_ entry templates; run figma-umg-naming after structure is ready.
---

# Figma UMG Structure Audit

Figma2UMG currently uses layer-name prefixes to decide UMG widget type, project texture references, Widget Blueprint boundaries, or asset-only exports. Structure recommendations should prepare nodes for prefixes such as `PNL_`, `IMG_`, `IMG_T_`, `Image_`, `BTN_`, `TXT_`, `LST_`, `TLV_`, and `WBP_`.

## Workflow

1. Use Figma MCP to fetch the target screenshot and node tree. Prefer `get_metadata` plus `get_screenshot`; use `get_design_context` when visual intent is unclear.
2. Inspect visible hierarchy before names. Hidden nodes are outside the default structure/naming scope unless the user explicitly asks to import hidden variants. Look for wrappers, construction frames, mixed visual/runtime responsibilities, IMG_ UImage candidates, IMG_T_ project texture references, direct T_ / Image_ image-source children, Image_ asset-only texture candidates, WBP_ Widget Blueprint boundaries, clickable hit areas, list entry templates, progress fills, tab groups, and repeated row patterns.
3. If the user provides or confirms a simulated/approved target tree, treat that tree as the implementation spec. Audit current Figma only against that tree; do not reinterpret, rename differently, or add extra runtime/image-source wrappers that are not in the approved tree unless a Figma2UMG rule makes them mandatory. If such a mandatory deviation exists, report it explicitly before applying or recommending it.
4. Propose structure changes only. Do not output final `RenameMap` unless the user explicitly asks; this skill runs before `figma-umg-naming`.
5. Return an audit table: `NodeId | CurrentName | CurrentType | Issue | RecommendedStructure | PrefixIntent | Priority | Reason`.
6. Include a short "After Structure" checklist naming which nodes should later be passed to `figma-umg-naming`.

## Audit Rules

- Prefer prefix-ready hierarchy. Recommend structures that can later be named with `PNL_`, `IMG_`, `Image_`, `BTN_`, `TXT_`, `LST_`, `TLV_`, and related current prefixes.
- When working from an approved tree, preserve its names, parent-child relationships, and intended omissions. Do not add helper nodes such as `Image_...` sources or extra `PNL_` wrappers just because they might be useful if the approved tree intentionally omitted them. Keep visible controls that are not in the approved tree separate as explicit extras instead of folding them into the confirmed structure.
- Runtime widget semantics should live on the smallest Figma node that should become that UMG widget.
- For Figma2UMG screens that use this convention, treat `PNL_Content` as the main content panel for the screen. Do not add or preserve an extra total-screen wrapper such as `PNL_Main` under `PNL_Content` just to mount the whole interface subtree. If such a wrapper exists only as a main container, recommend moving its visible children directly under `PNL_Content` and deleting or unprefixing the wrapper.
- Use lower-level `PNL_` containers only when they represent real layout or semantic regions, such as `PNL_BackgroundLayer`, `PNL_TopBar`, `PNL_LeftPanel`, `PNL_Center...`, `PNL_DetailPanel`, or `PNL_FooterActions`.
- Exclude hidden nodes from default structure recommendations and naming handoff. If a hidden node already has an import prefix and may be imported unintentionally, flag it separately instead of treating it as part of the visible runtime hierarchy.
- `IMG_` on a Frame/Group/vector/shape means a runtime UImage widget. It is not a generic asset-generation directive for the whole subtree. Give each `IMG_` node a clear image source: `IMG_T_...` for an existing `/Game` Texture2D, a direct `T_...` child for an existing project texture, a direct `Image_...` child for a generated asset-only texture, or valid fills when the importer can use them.
- `IMG_T_...` references an existing project texture whose asset name is the `T_...` suffix after `IMG_` is removed. For Group and Rectangle nodes it must not rely on its own fills to generate a texture or material. If it is a container, only `Image_` descendants should generate texture assets; ordinary descendants should not be relied on for asset/material generation.
- `Image_` means asset-only UI Texture2D export. It strips the `Image_` prefix from the generated Texture2D asset name and does not create a runtime UImage widget, Material, Font, Widget, or Widget Blueprint. Use it only for standalone texture resources or visual texture sources that should be referenced by other runtime widgets.
- Do not put runtime-interactive nodes, dynamic text, bound values, list entries, or buttons under an `IMG_` UImage node if they must remain runtime widgets. Split them out as siblings or overlay children under a runtime panel.
- Do not put required runtime children under an `Image_` node; they will not become runtime widgets. Split runtime content out and keep `Image_` visual-only.
- `WBP_` nodes are Widget Blueprint boundaries. Figma2UMG reuses an existing same-name Widget Blueprint when present, otherwise it generates a child Widget Blueprint and attaches it into the parent tree. Do not use `WBP_` for ordinary runtime controls.
- `LST_` and `TLV_` use the first child as the EntryWidget blueprint template. The first child should be a complete reusable item template, usually a `PNL_...Item`; later children must not be required as visible main-hierarchy controls.
- `BTN_` should be the outer hit-area Frame/Group. Its children should be `TXT_`, `IMG_`, and layout/content panels, never another `BTN_`.
- Use `PNL_` containers for free-positioned runtime grouping. Split mixed visual/runtime nodes into `IMG_` background/decor UImages with clear texture sources plus `PNL_` runtime content when the visual is complex.
- Use `VBX_` and `HBX_` for real vertical/horizontal layout. If the same node also carries complex background, rounding, stroke, or mask art, split visual art into `IMG_` and keep layout in `VBX_` or `HBX_`.
- Keep text that needs runtime binding as `TXT_`, not under an `IMG_` UImage or asset-only `Image_`.
- Use `PBR_` for runtime progress bars; keep visual-only track/fill art separate or static only when it does not need runtime-driven fill behavior.
- Repeated rows/items should use one consistent first-child template pattern.
- Account for importer rounding: final position and size are rounded to integers, so flag subpixel spacing, hairline offsets, or tiny fractional gaps when visual fidelity depends on them.
- Meaningless wrapper Frames should be removed, flattened, or left unprefixed only when they are intentionally used as non-runtime organization, part of an `IMG_` image-source structure, or part of a `WBP_` boundary.

## Output

Use priorities:
- `P0` blocks correct import semantics or loses runtime behavior, e.g. interactive/text-bound nodes under `IMG_`, required runtime content under `Image_`, misuse of `WBP_` as an ordinary runtime control, wrong `LST_`/`TLV_` first child, or nested buttons.
- `P1` imports but creates wrong or fragile hierarchy/visual behavior, e.g. an unnecessary `PNL_Main` wrapper under `PNL_Content`, mixed visual/runtime containers, inconsistent list item template, button hit area not covering intended area, or fractional layout likely affected by rounding.
- `P2` reduces noise, e.g. redundant wrappers, construction layers, or decorative fragments that could be represented by one `IMG_` UImage with a clear source.

```text
NodeId | CurrentName | CurrentType | Issue | RecommendedStructure | PrefixIntent | Priority | Reason
381:4 | PNL_Main | Frame | Redundant total-screen wrapper under PNL_Content | Move its visible children directly under PNL_Content, then delete or unprefix PNL_Main | PNL_Content root convention | P1 | PNL_Content is the main content panel; lower panels should represent real regions, not another whole-screen container.
381:93 | LST_Tab | Frame | ListView needs a complete first-child entry template | Keep LST_Tab as list container; make its first child PNL_TabItem with the full tab item content | LST_ first-child entry template | P0 | Figma2UMG assigns the first child as EntryWidgetClass.
381:5 | IMG_T_CharacterInfoBg | Frame | UImage references an existing project texture | Keep IMG_T_CharacterInfoBg as the UImage node; if generated sub-assets are required, only Image_ descendants should create them | IMG_T_ project texture reference | P1 | IMG_T_ binds T_CharacterInfoBg and should not generate its own material or texture.
381:6 | Image_EditIcon | Vector | Asset-only texture node must not own runtime content | Keep Image_EditIcon as a standalone visual texture source; move any runtime text/buttons to siblings under a runtime panel | Image_ UI Texture2D export | P0 | Image_ generates only a UI texture asset, not runtime widgets or materials.
381:2 | WBP_CharacterInfoNew | Frame | WBP_ creates or reuses a child Widget Blueprint boundary | Keep WBP_CharacterInfoNew only where a separate Widget Blueprint is intended; otherwise use a normal runtime root such as PNL_Root | WBP_ Widget Blueprint boundary | P1 | WBP_ is not an ordinary runtime widget prefix.
```

After the table, include:

```text
After Structure Checklist:
- Run figma-umg-naming on the updated file.
- Verify `PNL_Content` is the main content root and there is no extra `PNL_Main`-style wrapper under it.
- Verify visible screen regions are direct children of `PNL_Content` or meaningful lower-level panels, not hidden inside a generic all-content wrapper.
- Verify hidden nodes are excluded from the default structure/naming scope unless the user explicitly wants them imported.
- If an approved tree was provided, verify current visible UMG-prefixed nodes against it for missing nodes, parent mismatches, and extra visible prefixed controls before final handoff.
- Verify every IMG_ node has a clear UImage source: `IMG_T_...`, a direct `T_...` child, a direct `Image_...` child, or valid fills.
- Verify every `IMG_T_...` container only depends on `Image_` descendants for generated texture assets; ordinary descendants should not be required for resource generation.
- Verify every Image_ node is asset-only and does not own required runtime widgets.
- Verify WBP_ nodes are used only where a separate child/page Widget Blueprint is intended.
- Verify each LST_/TLV_ first child is a complete entry template.
- Verify BTN_ nodes are outer hit areas and contain no nested BTN_.
- Verify runtime text remains TXT_ and is not placed under IMG_ or Image_ nodes that would prevent it from becoming a runtime TextBlock.
- Check fractional positions/sizes that may shift after rounding.
```

For a reusable prompt to give another model or agent, read [references/structure-audit-prompt.md](references/structure-audit-prompt.md).
