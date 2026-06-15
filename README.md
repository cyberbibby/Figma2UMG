# Figma2UMG

**Import your Figma designs directly into Unreal Engine UMG** — turning your Figma frames into usable UMG widgets automatically.

---

## 🚀 Overview

**Figma2UMG** is an Unreal Engine plugin developed by Buvi Games that brings your Figma UI designs into Unreal with just a few clicks. It’s designed to accelerate UI prototyping and align design and development workflows by translating visual layouts into native UMG components.

Originally released on the Epic Marketplace, this plugin is now **open source and free to use** — with continued support for existing users.

---

## ✨ Features

- 📦 Convert Figma frames into UMG widgets
- 🧩 Preserves hierarchy and nesting of UI elements
- 🔤 Text and shape layers supported
- 🎨 Basic styling: fills, positions, sizes
- 📁 Generates Blueprint UserWidgets for rapid iteration

---

## 📥 Installation

### 🔹 Option 1: From Source (GitHub)
1. Clone or download this repository into your Unreal project’s `Plugins/` folder.
2. Launch Unreal Engine.
3. Enable the **Figma2UMG** plugin from the **Plugins** browser.
4. Restart Unreal if prompted.

### 🔹 Option 2: From FAB
Install the plugin directly from FAB:
👉 [https://www.fab.com/listings/0e0d4d1f-702f-4b3b-96c1-01c0fcac7823](https://www.fab.com/listings/0e0d4d1f-702f-4b3b-96c1-01c0fcac7823)

### 🔹 Optional: Codex Skills
This plugin includes Codex skills under `.codex/skills/` for preparing Figma files with the same prefix rules used by the importer:

- `figma-umg-structure-audit`
- `figma-umg-naming`

To install them for Codex:

```bash
mkdir -p "$HOME/.codex/skills"
cp -R Plugins/Figma2UMG/.codex/skills/figma-umg-structure-audit "$HOME/.codex/skills/"
cp -R Plugins/Figma2UMG/.codex/skills/figma-umg-naming "$HOME/.codex/skills/"
```

If you are developing the plugin and want the skills to stay in sync with this checkout, use symlinks instead:

```bash
mkdir -p "$HOME/.codex/skills"
ln -sfn "$(pwd)/Plugins/Figma2UMG/.codex/skills/figma-umg-structure-audit" "$HOME/.codex/skills/figma-umg-structure-audit"
ln -sfn "$(pwd)/Plugins/Figma2UMG/.codex/skills/figma-umg-naming" "$HOME/.codex/skills/figma-umg-naming"
```

---

## 🧭 Figma Layer Naming Rules

Figma2UMG uses Figma layer-name prefixes to decide which UMG widget or asset should be generated. Use the format:

```text
{PREFIX}_{SemanticName}
```

Examples: `PNL_Content`, `IMG_T_CharacterInfoBg`, `Image_EditIcon`, `BTN_Confirm`, `TXT_TabName`, `LST_Tab`, `PNL_TabItem`.

Prefix matching is case-insensitive. The import window keeps a configurable widget-prefix mapping table, so these defaults can be overridden when needed.

### Default UMG Prefixes

| Prefix | Generated UMG type |
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

### Special Prefixes

| Prefix | Import behavior |
| --- | --- |
| `IMG_T_` | Creates a runtime UImage and binds it to an existing `/Game` `UTexture2D` whose asset name is the `T_...` suffix after `IMG_` is removed. For example, `IMG_T_Icon_Back` looks for `T_Icon_Back`. |
| `Image_` | Generates only an asset-only UI Texture2D. The generated texture name strips the `Image_` prefix. It does not create a runtime widget, material, font, widget, or widget blueprint. |
| `WBP_` | Marks a Widget Blueprint boundary. The importer reuses an existing same-name Widget Blueprint when present; otherwise it generates a child Widget Blueprint and attaches it into the parent tree. |

### Structure Notes

- Put the prefix on the smallest Figma node that should become that widget or asset.
- Use `TXT_` for runtime text. Do not place bound text, buttons, list entries, or other runtime controls under `IMG_` / `Image_` nodes when they must remain editable UMG widgets.
- Use `IMG_` for runtime UImage widgets. If the image source comes from another layer, prefer a direct `T_...` child for an existing project texture or a direct `Image_...` child for a generated texture asset.
- Use `LST_` and `TLV_` for ListView / TileView widgets. Their first direct child is used as the EntryWidget blueprint template and assigned to `EntryWidgetClass`.
- Use `BTN_` on the outer clickable hit-area Frame or Group. Do not nest `BTN_` inside another `BTN_`.
- Final widget positions and sizes are rounded to integer values during import.

The bundled Codex skills in `.codex/skills/` use these same rules to audit and rename Figma files before import.

---

## 🎮 Usage

1. In Unreal, open the **Import Figma file** from the Content Browser's Context Menu
2. Enter your **Figma Personal Access Token** and **File ID**. - https://www.figma.com/developers/api#access-tokens
3. (Optional) You can add IDs to import only elements of your file.
4. Set your desired import path in the Content Browser (e.g., `/Game/UI/Figma/`).
5. Click **Import**.

Once done, the plugin will generate UMG widgets representing your Figma layout, ready to be used or customized in Blueprint.

---

## 📚 Resources

- 📄 **Documentation**: [https://www.buvi.games/figma2umg](https://www.buvi.games/figma2umg)
- 🔌 **Figma Plugin**: [https://www.figma.com/community/plugin/1368487806996965174/figma2umg-unreal-importer](https://www.figma.com/community/plugin/1368487806996965174/figma2umg-unreal-importer)
- 💬 **Support**: [figma2umg@buvi.games](mailto:figma2umg@buvi.games)

---

## 🐞 Bug Reports

Please send an email to [figma2umg@buvi.games](mailto:figma2umg@buvi.games) and include:
- A **Figma file** for testing  
- If the issue is related to layout rendering, include a screenshot or UMG showing the **expected result**

This will help reproduce and fix the problem more efficiently.

---

## 📌 Roadmap & Community

Planned improvements include:

- Multi-frame support
- Better layout handling (e.g., Horizontal/Vertical/Grid Boxes)
- Font and style syncing
- Text formatting (justification, wrapping)
- Outline and gradient support

**Want to help?** Contributions are welcome — feel free to submit issues, feature requests, or pull requests.

---

If you purchased the plugin via the Epic Marketplace, don’t worry — you’ll continue receiving **personal support** from me.

If you'd like to support ongoing development, you can still join the [Patreon](https://www.patreon.com/) 💛

---

## 🙌 Special Thanks

Big thanks to:
- Everyone who purchased Figma2UMG on the Marketplace
- All my Patreon supporters
- Anyone giving feedback or spreading the word

You helped make this possible.

---

## 📝 License

This plugin is licensed under the **MIT License** — see [`LICENSE`](LICENSE) for details.

---

**Built with ❤️ by Buvi Games**
