---
name: figma-umg-naming
description: Apply or audit Figma2UMG prefix names on structure-ready Figma layers for Unreal UMG import. Use for RenameMap generation or checking names such as PNL_, IMG_, Image_, BTN_, TXT_, LST_, TLV_, and WBP_; use figma-umg-structure-audit first for hierarchy changes.
---

# Figma UMG Naming

## Workflow

1. Use Figma MCP to fetch both the target screenshot and metadata / node tree. Prefer `get_metadata` plus `get_screenshot`; use `get_design_context` when visual details or text content are needed.
2. Audit visible nodes against the current Figma2UMG prefix map. Hidden nodes are outside the default naming scope unless the user explicitly asks to import hidden variants. Treat layer names as mapping directives, not visual labels.
3. If the user provides or confirms a simulated/approved target tree, use that tree as the naming spec. Match its exact names and parent-child intent; do not invent alternate names or add prefixes for nodes intentionally omitted from the tree. Report extra visible prefixed controls separately instead of silently merging them into the approved structure.
4. Rename only the node that should become the runtime UMG widget or an explicit `Image_` asset-only texture resource. If the Figma layer hierarchy itself is wrong, flag `NeedsStructureAudit` instead of inventing names that hide the problem.
5. Propose changes in a table: `NodeId | CurrentName | SuggestedName | Prefix | UMGType | Confidence | Reason`.
6. Emit a `RenameMap` JSON containing only changed nodes with confidence >= 0.80.
7. Do not apply renames unless the user explicitly asks. If applying renames in Figma, load and follow the `figma-use` skill before calling `use_figma`.

## Naming Format

Use `{PREFIX}_{EnglishPascalCaseSemanticName}`, for example `PNL_Content`, `IMG_Bg`, `Image_EditIcon`, `BTN_Confirm`, `TXT_TabName`, `LST_Tab`, or `PNL_TabItem`.

## Default Prefix Map

| Prefix | UMG type |
| --- | --- |
| `PNL_` | CanvasPanel |
| `VBX_` | VerticalBox |
| `HBX_` | HorizontalBox |
| `OVR_` | Overlay |
| `WPB_` | WrapBox |
| `UGP_` | UniformGridPanel |
| `GDP_` | GridPanel |
| `WSW_` | WidgetSwitcher |
| `BDR_` | Border |
| `SIZ_` | SizeBox |
| `SCL_` | ScaleBox |
| `SFZ_` | SafeZone |
| `MNA_` | MenuAnchor |
| `NSL_` | NamedSlot |
| `BLR_` | BackgroundBlur |
| `INB_` | InvalidationBox |
| `RTB_` | RetainerBox |
| `TBA_` | WindowTitleBarArea |
| `SCR_` | ScrollBox |
| `SBR_` | ScrollBar |
| `TXT_` | TextBlock |
| `ETXT_` | RichTextBlock |
| `EDT_` | EditableText |
| `EDB_` | EditableTextBox |
| `MLT_` | MultiLineEditableText |
| `MLB_` | MultiLineEditableTextBox |
| `IMG_` | Image |
| `BTN_` | Button |
| `CHK_` | CheckBox |
| `CMB_` | ComboBoxString |
| `PBR_` | ProgressBar |
| `SLD_` | Slider |
| `SPN_` | SpinBox |
| `KEY_` | InputKeySelector |
| `THB_` | Throbber |
| `CTH_` | CircularThrobber |
| `SPC_` | Spacer |
| `LST_` | ListView |
| `TLV_` | TileView |

`WBP_` is a reserved Widget Blueprint boundary prefix. Figma2UMG reuses an existing same-name Widget Blueprint when present, otherwise it generates a child Widget Blueprint and attaches it into the parent tree. It is not an ordinary runtime widget mapping.

## Special Asset Prefixes

| Prefix | Import behavior |
| --- | --- |
| `Image_` | Asset-only UI Texture2D export |

`IMG_` is a runtime UImage widget prefix. `IMG_T_AssetName` means the UImage should look up an existing `/Game` `UTexture2D` whose asset name is exactly the `T_...` suffix after `IMG_` is removed. For example, `IMG_T_Icon_Back` binds the UImage to `T_Icon_Back` if that texture exists.

`Image_` is not a runtime UMG widget prefix. Figma2UMG generates only a UI TextureGroup texture asset for nodes named with `Image_`; it strips the `Image_` prefix from the generated Texture2D asset name and does not generate Material, Font, Widget, or Widget Blueprint assets for that node. Use `Image_` only for standalone texture resources that should be referenced elsewhere or consumed as visual texture sources.

## Core Rules

- Put the prefix on the smallest node that should become that UMG widget. Do not put runtime widget prefixes on meaningless wrapper frames.
- When an approved tree exists, preserve it as the source of truth for names and intentionally omitted nodes. Do not add helper names such as extra `Image_...` sources, background `IMG_...` nodes, or wrapper `PNL_...` nodes unless the approved tree includes them or Figma2UMG import would otherwise be invalid; flag any such required deviation as a structure concern.
- For Figma2UMG screens that use this convention, the main content root should be `PNL_Content`. Do not suggest or preserve `PNL_Main` as an additional all-content wrapper under `PNL_Content`; flag `StructureConcern` so its visible children can be moved directly under `PNL_Content`.
- Do not include hidden nodes in the default `RenameMap`. If a hidden node appears to be an intentional import/state variant, report it separately and ask for explicit scope instead of naming it by default.
- Preserve existing correct prefix names. Rename only nodes with no prefix, wrong widget prefixes, or unstable semantic names.
- Use `PNL_` for free-positioned runtime containers, `VBX_` for vertical layout, `HBX_` for horizontal layout, `OVR_` for overlays, `SCR_` for scroll areas, and `WSW_` for widget switchers.
- Use `TXT_` for Figma `TEXT` nodes, including labels, values, and glyph icons such as `+`, `!`, `?`, `x`, arrows, or pencil characters. Use `ETXT_` only for rich text.
- Use editable text prefixes only for true input controls: `EDT_`, `EDB_`, `MLT_`, or `MLB_`.
- Put `BTN_` on the outermost clickable Frame/Group, not on its inner text or icon. Button children should usually be `TXT_`, `IMG_`, or layout/content panels; never nest `BTN_` inside another `BTN_`.
- Use `IMG_` for runtime UImage widgets: images, vectors, shapes, avatars, icons, and visual-only Frame/Group nodes that should appear as a UImage in UMG.
- Use `IMG_T_...` when the UImage should bind an existing project texture. For `Group` and `Rectangle` nodes, `IMG_T_...` should only look up that texture and must not rely on the node's own fills to generate a texture or material. If it is a container, only `Image_` descendants are allowed to generate texture assets.
- If an `IMG_` node gets its image source from children, prefer a direct `T_...` child for an existing project texture or a direct `Image_...` child for a generated asset-only texture. Do not expect ordinary children under `IMG_` to become normal runtime widgets.
- Use `Image_` only for asset-only UI texture exports. Do not use `Image_` when the node should become a runtime UImage widget, a runtime container, text, material, font, or child Widget Blueprint.
- Do not put runtime text, bound values, buttons, list entries, or other interactive children under an `IMG_` UImage node unless the structure is explicitly split so those controls live in a separate runtime container. Flag `NeedsStructureAudit` if that is required visually.
- Use `WBP_` only for child/page Widget Blueprint boundaries. Do not recommend `WBP_` for ordinary runtime controls.
- Use `LST_` and `TLV_` for list/tile view widgets. Their first child is the EntryWidget blueprint template source, for example `LST_Tab -> PNL_TabItem -> TXT_TabName`; that first child should not be treated as a normal main-tree child.
- Put `PBR_` on the full progress track. Keep separate static visual fill/helper art as an `IMG_` UImage with a clear texture source, or flag structure concerns if the fill must be runtime-driven.
- For tab bars, use `LST_` / `TLV_` when the tabs are data-driven entries, `HBX_` for a static horizontal tab row, or `PNL_` for free-positioned tabs. Each clickable static tab needs an outer `BTN_`; list entries should use a complete first-child entry template.
- For "display text + edit icon" fields, use `PNL_` or `BDR_` for the field container, `TXT_` for display text, and an outer `BTN_` around the edit hit area.
- Warning/help glyphs are `BTN_` only when they trigger tooltip/help/details; otherwise use `TXT_` for glyph text or `IMG_` for image/vector icons.

## Structure Boundary

This skill should not design a new Figma hierarchy. If the correct mapping requires moving children, flattening wrappers, splitting a styled layout node into `BDR_ + HBX_` / `BDR_ + VBX_`, or changing which node contains the hit area, return a short `StructureConcern` note and recommend running `figma-umg-structure-audit`.

Also return `StructureConcern` when a `PNL_Main`-style wrapper exists under `PNL_Content`. The structure audit should move the wrapper's visible children directly under `PNL_Content` and remove or unprefix the extra wrapper.

Also return `StructureConcern` when the current names diverge from a user-approved target tree because a hierarchy change, detach, flatten, or wrapper removal is needed to make the names match that tree.

## Output Template

```text
NodeId | CurrentName | SuggestedName | Prefix | UMGType | Confidence | Reason
381:4 | Content | PNL_Content | PNL_ | CanvasPanel | 0.96 | Free-positioned root content container.
381:5 | Bg | IMG_T_CharacterInfoBg | IMG_ | Image | 0.94 | Runtime UImage should bind an existing project texture named T_CharacterInfoBg.
381:6 | IconSource | Image_EditIcon | Image_ | UI Texture2D Asset | 0.90 | Standalone texture resource only; generated asset name is EditIcon and it is not a runtime widget.
381:93 | Tab | LST_Tab | LST_ | ListView | 0.95 | Data-driven tab list; first child supplies the entry widget.
381:94 | TabItem | PNL_TabItem | PNL_ | CanvasPanel | 0.90 | First child of LST_Tab is the entry root template.
381:104 | TabName | TXT_TabName | TXT_ | TextBlock | 0.98 | Text label inside the tab entry template.
```

```json
{
  "381:4": "PNL_Content",
  "381:5": "IMG_T_CharacterInfoBg",
  "381:6": "Image_EditIcon",
  "381:93": "LST_Tab",
  "381:94": "PNL_TabItem",
  "381:104": "TXT_TabName"
}
```

Structure concern example:

```text
NodeId | CurrentName | StructureConcern | RecommendedAuditFocus
381:10 | PNL_Main | Redundant all-content wrapper under PNL_Content | Move visible children directly under PNL_Content, then delete or unprefix PNL_Main.
```

For the full reusable prompt to give another model or agent, read [references/naming-prompt.md](references/naming-prompt.md).
