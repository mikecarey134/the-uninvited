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

- **UP** — `approach/normal.png`
- **RIGHT** — `approach/force.png`
- **LEFT** — `approach/trade.png` or `approach/companion-delegate.png`. These are contextual Guile sub-types; which one shows depends on whether the target is an NPC or an object.
- **DOWN** — the specific skill icon from `skills/` for the action in context (e.g. `skills/lockpicking.png`, `skills/engineering.png`). It does not use `modes/skill.png`.
- The D-pad frame itself is drawn in-engine with lines and shapes; it is not an art asset.

## Modes

`modes/` icons label the four approach categories (Normal, Force, Skill, Guile) in menus, tooltips and the PC right-click context panel. They are not used in the D-pad HUD.

## Naming

All lowercase, hyphens between words, no prefix that repeats the folder name (`aegis/strength.png`, not `aegis/aegis-strength.png`).

## Resolution

These are master-resolution source images: 1254px for aegis, skills, modes and rings; 1024px for approach; 256–1254px for interactables. Runtime downscaled copies will be added as a separate future task.
