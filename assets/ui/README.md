# UI assets

## Folders

- `icons/aegis/` — the 5 AEGIS attribute icons (agility, endurance, guile, intelligence, strength).
- `icons/skills/` — the 10 skill icons (chemistry, construction, cooking, electronics, engineering, lockpicking, medicine, smithing, speech, survivalism).
- `icons/approach/` — the D-pad HUD set: normal, force, trade, companion-delegate.
- `icons/modes/` — generic category labels for the four approaches: normal, force, skill, guile.
- `icons/rings/` — the colored ring elements (cyan, gold, green, red).
- `icons/interactables/` — object icons (doors, crates, terminal, vehicle, etc.). Reserve assets, not currently assigned to any system.
- `hud/`, `menus/`, `overlays/`, `portraits/`, `status/` — empty placeholders for future art. Existing HUD art still lives in `assets/hud/`.

## D-pad (runtime)

- **UP** (Skill, cyan ring) — the specific skill icon from `skills/` for the object's first skill approach (e.g. `skills/lockpicking.png`, `skills/engineering.png`). It does not use `modes/skill.png`.
- **RIGHT** (Guile, green ring) — `approach/trade.png` or `approach/companion-delegate.png`. These are contextual Guile sub-types; which one shows depends on whether the target is an NPC or an object.
- **DOWN** (second Skill, cyan ring) — the skill icon for the object's second skill approach, if it has one (e.g. `skills/electronics.png` for a keypad); otherwise the slot is empty.
- **LEFT** (Force, red ring) — `approach/force.png`
- **Normal is not on the D-pad.** It is the default action: X on a controller, E or left-click on PC, with nothing picked on the D-pad. `approach/normal.png` is not shown in the D-pad.
- The D-pad frame itself is drawn in-engine with lines and shapes; it is not an art asset.

## Modes

`modes/` icons label the four approach categories (Normal, Force, Skill, Guile) in menus, tooltips and the PC right-click context panel. They are not used in the D-pad HUD.

## Naming

All lowercase, hyphens between words, no prefix that repeats the folder name (`aegis/strength.png`, not `aegis/aegis-strength.png`).

## Resolution

These are master-resolution source images: 1254px for aegis, skills, modes and rings; 1024px for approach; 256–1254px for interactables. Runtime downscaled copies will be added as a separate future task.
