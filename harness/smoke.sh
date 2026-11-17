#!/usr/bin/env bash
# The smoke run: one fixed scenario through harness/play.sh, for a code-changing step of the clean-up.
#
# Usage: harness/smoke.sh [--long] [--skip STEP[,STEP...]] [-L program | -X exe | -n] [-S slot] [-w dir] [-o dir]
#   -X exe / -L program   the program (default build/noexe/FalloutNV.exe); -n the original game (to tell the port's
#                         faults from the game's own behaviour); -w, -S, -o as harness/play.sh's
#   --long                the long walk: the short one, then the steps below (about 20 minutes, two game runs)
#   --skip                long-walk steps left out, by name (container,drop,vats,grenade,kill,levelup,settings,death,
#                         barter,dialogue)
#
# The short walk (about 5 minutes): start; the main menu; `coc GSDocMitchellHouse` (an interior); walk; a young
# bighorner brought in front of the player (restrained), a 9mm pistol drawn (R) and fired at it three times (hit);
# open and close the Pip-Boy; `save smoke1`; quit to the main menu (the pause menu); load that save; quit (`qqq`).
# The target is a creature: Doc Mitchell, who lives there, takes no damage from the player's bullets, in the
# original game as well.
# The long walk goes on after the load, each step setting up what it needs: container (Take All from Doc's dresser
# holding one stimpak), drop (a stimpak dropped, then picked up again from the floor), vats (one queued shot at the
# bighorner), grenade (a frag grenade thrown at its feet), kill (the bighorner shot dead, its corpse looted), levelup
# (`player.advlevel`, the level-up menu worked through and closed), settings (the difficulty raised in the pause
# menu's Settings), death (`player.kill` after a save, the death menu's Continue: the save loads), barter (one stack
# bought from Chet in the General Store), dialogue (Chet held in front of the player, E pressed at him, his greeting
# clicked through, a topic chosen, the reply on screen), then quit; and a second run with the first one's My Games
# (run.sh -k), to read the setting back.
# Prints one line per step, `STEP reached|NOT reached|skipped: evidence`, then a verdict (harness/play/smoke_check.py).
# The evidence is the console's own output (`scof` into the run tree, one file per step, moved to the output folder:
# GetInCell, positions, IsWeaponOut, inventory counts, actor values, GetDead, GetLevel, GetINISetting), the save file,
# FalloutPrefs.ini, the game's exit status, the screen's text (tesseract) for the dialogue; the Pip-Boy by its
# screenshots' change. A screenshot of every step is in the output folder (OUT/STEP.png). A crash log fails the run.
# The slot's run tree must exist (one harness/run.sh run in the slot makes it). Default slot: as harness/play.sh.
# Exit status: 0 when every step is reached and there is no crash log, 1 otherwise, 64/69 usage or setup.
set -u
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
wt=$(cd "$here/.." && pwd)
prog=()
slot=${FNVD_SLOT:-}
out=""
long=0
skip=","
opts=()
while [[ $# -gt 0 ]]; do
	case $1 in
	--long) long=1 ;;
	--skip) skip+="${2:-},"; shift ;;
	--skip=*) skip+="${1#--skip=}," ;;
	*) opts+=("$1") ;;
	esac
	shift
done
set -- "${opts[@]}"
while getopts "X:L:nS:w:o:" opt; do
	case $opt in
	X | L) prog=(-"$opt" "$OPTARG") ;;
	n) prog=(-n) ;;
	S) slot=$OPTARG ;;
	w) wt=$(cd "$OPTARG" && pwd) ;;
	o) out=$OPTARG ;;
	*) sed -n '2,/^set -u/p' "$0" | sed '$d' >&2; exit 64 ;;
	esac
done
[[ ${#prog[@]} -eq 0 ]] && prog=(-X build/noexe/FalloutNV.exe)
[[ -z $slot ]] && { [[ ${prog[0]} == -L ]] && slot=G || slot=7; }
envdir=$(realpath -m "$wt/env")
run=$envdir/run-$slot
[[ -d $run ]] || { echo "smoke: no run tree $run: one harness/run.sh run in slot $slot creates it" >&2; exit 69; }
out=${out:-$envdir/out/smoke-$slot-$(date +%Y%m%d-%H%M%S)}
mkdir -p "$out"
out=$(cd "$out" && pwd)
if [[ ${prog[0]} == -L ]]; then
	mygames="$envdir/linux-data-$slot/FalloutNV/Documents/My Games/FalloutNV"
else
	mygames="$envdir/prefix-$slot/drive_c/users/$USER/Documents/My Games/FalloutNV"
fi

# The console batch files (`bat NAME`: NAME.txt in the run tree, CRLF lines as the game's own tools write them).
batch() {
	local name=$1
	shift
	printf '%s\r\n' "$@" >"$run/$name.txt"
}
NPC=107077    # GSBighorner01Ref, a young bighorner (60 health points)
GUN=e3778     # WeapNV9mmPistol
AMMO=8ed03    # Ammo9mm
STIM=15169    # Stimpak
GRENADE=4330  # WeapGrenadeFrag
DRESSER=103e37 # DocMitchellDresserRef
CHET=104c79   # GSChetRef, the General Store's trader
CELL=GSDocMitchellHouse
STORE=GSGeneralStore
DIFF='getinisetting "iDifficulty:GamePlay"'
# GetItemCount prints nothing in the console: counts come from showinventory. No tgm: god mode does not use ammo.
batch smoke_menu "player.getincell $CELL" "player.getpos x"
batch smoke_cell "player.getincell $CELL" "player.getpos x" "player.getpos y" "player.getpos z" \
	"player.additem $GUN 1" "player.additem $AMMO 50" "player.equipitem $GUN" "player.additem f 100" \
	"player.setav guns 100"
batch smoke_walk "player.getpos x" "player.getpos y" "player.getpos z"
# the target held still (restrained) in front of the player: brought to the player, who steps back
batch smoke_bring "prid $NPC" "setrestrained 1" "moveto player"
batch smoke_where "player.getpos x" "player.getpos y" "player.getpos z" "prid $NPC" "getpos x" "getpos y" \
	"getpos z" "getav health" "player.showinventory"
batch smoke_fire "player.isweaponout" "player.showinventory" "prid $NPC" "getav health"
batch smoke_saved "player.showinventory"
batch smoke_load "player.getincell $CELL" "player.showinventory"
batch smoke_inv "player.showinventory"
rm -f "$run"/smoke_con_* "$run/smoke_aim.txt" "$run"/smoke_sweep*.txt
aim() { echo "sh:/usr/bin/python3 $here/play/smoke_aim.py $run/smoke_con_$1 $run/smoke_aim.txt ${2:-}"; }

# The console closes itself at a coc, stays open after a load from the main menu; `scof` ends at a cell load.
steps=(
	"shot:menu" "con:scof+smoke_con_menu;bat+smoke_menu" "sleep:1"
	"cn:coc+$CELL" "sleep:25" "shot:coc" "con:scof+smoke_con_cell;bat+smoke_cell" "sleep:1"
	"hold:w:2" "sleep:1" "con:bat+smoke_walk" "shot:walk"
	"con:bat+smoke_bring" "sleep:1" "hold:s:0.6" "sleep:1" "con:bat+smoke_where" "sleep:1"
	"$(aim cell 50)" "con:bat+smoke_aim"
	"tap:r" "sleep:4" "shot:aim" "mhold:0.3" "sleep:2" "mhold:0.3" "sleep:2" "mhold:0.3" "sleep:2" "shot:fire"
	"con:bat+smoke_fire" "sleep:1"
	"shot:pipboy_before" "hold:Tab:0.4" "sleep:3" "shot:pipboy" "hold:Tab:0.4" "sleep:3" "shot:pipboy_closed"
	"con:bat+smoke_saved;save+smoke1" "sleep:12" "shot:save" "con:player.additem+f+1" "sleep:1"
	# the pause menu: six Downs reach Quit, then Main Menu (the dialog's first choice)
	"hold:Escape:0.4" "sleep:2" "hold:Down:0.4" "sleep:1" "hold:Down:0.4" "sleep:1" "hold:Down:0.4" "sleep:1"
	"hold:Down:0.4" "sleep:1" "hold:Down:0.4" "sleep:1" "hold:Down:0.4" "sleep:1" "hold:Return:0.4" "sleep:2"
	"shot:quit_dialog" "hold:Return:0.4" "sleep:12" "shot:quit_menu" "con:scof+smoke_con_quit;bat+smoke_menu"
	"cn:load+smoke1" "sleep:30" "shot:load" "ct:scof+smoke_con_load" "ct:bat+smoke_load" "hold:grave:0.4"
	"sleep:1"
)

# The long walk's steps, each added unless --skip names it; each opens its own console output file.
wanted() { [[ $long == 1 && $skip != *",$1,"* ]]; }
add() { steps+=("$@"); }
# the bighorner brought in front of the player again, the player stepping back (SECS, default 0.6) and its angles
# set at it (smoke_aim.py; HEIGHT 0: its feet)
bring() {
	steps+=("con:bat+smoke_bring" "sleep:1" "hold:s:${3:-0.6}" "sleep:1" "con:bat+smoke_where" "sleep:1"
		"$(aim "$1" "${2:-50}")" "con:bat+smoke_aim")
}
# KEY held N times into the array named first. A menu loses the first key after it opens, and Up stops at the top
# row, so Up four times selects the top row whatever was selected
press() {
	local -n to=$1
	local i
	for ((i = 0; i < $3; i++)); do to+=("hold:$2:0.3" "sleep:1"); done
}
if wanted container; then
	batch smoke_container "player.showinventory" "prid $DRESSER" "removeallitems" "additem $STIM 1" "setownership" \
		"activate player"
	# the container menu opens with the player's column selected; A takes all and closes it
	add "con:scof+smoke_con_container;bat+smoke_container" "sleep:3" "shot:container" "hold:a:0.4" "sleep:2" \
		"shot:container_taken" "con:bat+smoke_inv" "sleep:1"
fi
if wanted drop; then
	batch smoke_drop "player.additem $STIM 1" "player.setangle x 0" "player.showinventory" "player.drop $STIM 1" \
		"player.showinventory" "player.getangle z"
	# the stimpak falls in front of the player and may roll: the player looks down at a few points around it
	# (smoke_sweep.py), pressing E at each, until one picks it up
	add "con:scof+smoke_con_drop;bat+smoke_drop" "sleep:3" \
		"sh:/usr/bin/python3 $here/play/smoke_sweep.py $run/smoke_con_drop $run/smoke_sweep"
	for i in 1 2 3 4 5 6 7 8; do
		add "con:bat+smoke_sweep$i" "sleep:0.5"
		[[ $i == 1 ]] && add "shot:drop"
		add "tap:e" "sleep:1"
	done
	add "con:bat+smoke_inv;player.setangle+x+0" "sleep:1"
fi
if wanted vats; then
	# the action points come back within seconds: their restore rate is set to 0 for the rest of the run. The
	# target is brought back to life (an earlier step may have killed it)
	batch smoke_vats "setgs fActionPointsRestoreRate 0" "prid $NPC" "resurrect" "player.equipitem $GUN" \
		"player.showinventory" "player.getav actionpoints"
	batch smoke_vats_after "player.showinventory" "player.getav actionpoints"
	# the first VATS of a game opens its help box (E closes it); a click queues a shot on the body part under the
	# cursor, E accepts
	add "con:scof+smoke_con_vats;bat+smoke_vats"
	bring vats
	add "tap:r" "sleep:3" "hold:v:0.4" "sleep:2" "shot:vats" "tap:e" "sleep:1" "click" "sleep:1" "shot:vats_queued" \
		"tap:e" "sleep:8" "shot:vats_fired" "con:bat+smoke_vats_after" "sleep:1"
fi
if wanted grenade; then
	# the target alive again; its health before the throw is the one smoke_where reads after it is brought in
	batch smoke_grenade "player.additem $GRENADE 2" "player.equipitem $GRENADE" "player.showinventory" "prid $NPC" \
		"resurrect"
	batch smoke_grenade_after "player.showinventory" "prid $NPC" "getav health" "player.equipitem $GUN"
	add "con:scof+smoke_con_grenade;bat+smoke_grenade"
	# thrown from about 500 units: at 250 the blast crippled the player (an x64 run), whose camera was then on the
	# floor, and the steps after it failed
	bring grenade 0 1.3
	add "sleep:2" "mhold:0.3" "sleep:4" "shot:grenade" "con:bat+smoke_grenade_after" "sleep:1"
fi
if wanted kill; then
	# the bighorner alive again, then (once brought in) with 10 health points, carrying two stimpaks to loot
	batch smoke_kill "prid $NPC" "resurrect" "player.equipitem $GUN"
	batch smoke_kill_set "prid $NPC" "restoreav health 1000" "setav health 10" "getav health" "getdead" \
		"additem $STIM 2"
	batch smoke_loot "prid $NPC" "getdead" "player.showinventory" "activate player"
	add "con:scof+smoke_con_kill;bat+smoke_kill"
	bring kill
	add "con:bat+smoke_kill_set" "sleep:1"
	add "tap:r" "sleep:3" "mhold:0.3" "sleep:2" "mhold:0.3" "sleep:2" "mhold:0.3" "sleep:3" "shot:kill" \
		"con:bat+smoke_loot" "sleep:3" "shot:loot" "hold:a:0.4" "sleep:2" "con:bat+smoke_inv" "sleep:1"
fi
if wanted levelup; then
	batch smoke_level "player.getlevel" "player.getav energyweapons" "player.getpos x" "player.getpos y"
	# skills: Down to Energy Weapons, Right until the points are spent, A; perks: Return on the first, A
	rights=()
	for _ in $(seq 15); do rights+=("hold:Right:0.2" "sleep:0.4"); done
	add "con:scof+smoke_con_levelup;bat+smoke_level;player.advlevel" "sleep:5" "shot:levelup" "hold:Down:0.3" \
		"sleep:1" "${rights[@]}" "hold:a:0.3" "sleep:2" "hold:Return:0.3" "sleep:1" "hold:a:0.3" "sleep:3" \
		"shot:levelup_closed" "hold:w:1" "sleep:1" "con:bat+smoke_level" "sleep:1"
fi
if wanted settings; then
	batch smoke_settings "$DIFF"
	# the pause menu: Continue, Save, Load, Settings, Help, Quit; Down skips a disabled row (a Linux run's Down x3
	# opened Help), so the Downs to Settings are counted from the enabled rows on the screen
	# (menu_downs.py). Its Settings: Gameplay first; Gameplay: Kill Cam Mode, Hardcore Mode, Difficulty (Right
	# raises it); Escape closes every menu
	add "con:scof+smoke_con_settings;bat+smoke_settings" "hold:Escape:0.4" "sleep:2"
	press steps Up 4
	add "shot:pause" "sh:n=\$(/usr/bin/python3 $here/play/menu_downs.py \$out/pause.png Settings Continue Save Load \
Settings Help Quit); echo \"settings: \$n Downs\"; for _ in \$(seq \$n); do k hold:Down:0.3; sleep 1; done"
	add "hold:Return:0.3" "sleep:2"
	press steps Up 4
	add "hold:Return:0.3" "sleep:2"
	press steps Up 4
	press steps Down 2
	add "hold:Right:0.3" "sleep:1" "shot:settings_changed" "hold:Escape:0.4" "sleep:2" "con:bat+smoke_settings" \
		"sleep:1"
fi
if wanted death; then
	batch smoke_death "player.showinventory" "save smoke2"
	batch smoke_death_kill "player.additem f 7" "player.showinventory" "player.kill"
	batch smoke_death_load "player.getincell $CELL" "player.showinventory"
	# the death menu (the pause menu with Continue, Settings, Quit): Continue loads the last save
	add "con:scof+smoke_con_death;bat+smoke_death" "sleep:12" "con:bat+smoke_death_kill" "sleep:15" "shot:death" \
		"hold:Return:0.4" "sleep:30" "shot:death_load" "cn:scof+smoke_con_death_load" "ct:bat+smoke_death_load" \
		"hold:grave:0.4" "sleep:1"
fi
if wanted barter || wanted dialogue; then
	add "cn:coc+$STORE" "sleep:25" "shot:store"
fi
if wanted barter; then
	# the player's column holds one 9mm round only, so its first row is known: Down selects it, Return offers it and
	# moves the selection to Chet's first row, Return there asks how many (A: all of them), A accepts the trade
	# (Chet's stack costs more than the round), X leaves the menu
	batch smoke_barter "player.removeallitems" "player.additem f 2000" "player.additem $AMMO 1" "player.showinventory" \
		"prid $CHET" "showbartermenu"
	add "con:scof+smoke_con_barter;bat+smoke_barter" "sleep:3" "shot:barter" "hold:Down:0.3" "sleep:1" \
		"hold:Return:0.3" "sleep:2" "hold:Return:0.3" "sleep:2" "hold:a:0.3" "sleep:2" "shot:barter_offer" \
		"hold:a:0.3" "sleep:2" "shot:barter_done" "hold:x:0.3" "sleep:2" "con:bat+smoke_inv" "sleep:1"
fi
if wanted dialogue; then
	batch smoke_dialogue "prid $CHET" "setrestrained 1" "moveto player"
	batch smoke_dialogue_where "player.getpos x" "player.getpos y" "player.getpos z" "prid $CHET" "getpos x" \
		"getpos y" "getpos z"
	batch smoke_dialogue_after "prid $CHET" "gettalkedtopc" "setrestrained 0"
	# Chet held still in front of the player, who presses E at him as a player does: the console's `activate player`
	# runs his greeting's conditions but opens no dialogue, in the original game as well (the menu request is dropped
	# while the console is open). Without game audio a response line stays until it is clicked, so each greeting line
	# is clicked away (a click skips a voiced line too, so the steps are the same with audio; the game's cursor
	# ignores the pointer's absolute moves and stays mid-screen): Chet's first greeting has three lines, then Down,
	# Return choose a topic. A menu takes a click only once the cursor has moved since the last menu worked with
	# keys (the pause menu's Downs before the load), in the original game as well, so the pointer is moved a little
	# and back first
	add "con:bat+smoke_dialogue" "sleep:2" "hold:s:0.25" "sleep:1" \
		"con:scof+smoke_con_dialogue;bat+smoke_dialogue_where" "sleep:1" "$(aim dialogue 100)" "con:bat+smoke_aim" \
		"sleep:1" "tap:e" "sleep:4" "shot:dialogue" "rel:8:0" "sleep:0.5" "rel:-8:0" "sleep:1"
	for i in 1 2 3; do add "click" "sleep:2"; done
	add "shot:dialogue_topics" "hold:Down:0.3" "sleep:1" "hold:Return:0.3" "sleep:3" "shot:dialogue_reply" \
		"con:bat+smoke_dialogue_after" "sleep:1"
fi
steps+=("cn:qqq" "sleep:15")
# the run's limit: the main menu's wait, the sleeps, about 3 s per other step, and a margin
secs=0
for s in "${steps[@]}"; do
	if [[ $s == sleep:* ]]; then
		v=${s#sleep:}
		secs=$((secs + ${v%%.*}))
	else
		secs=$((secs + 3))
	fi
done
secs=$((70 + secs * 5 / 4 + 30))

IFS='|'
"$here/play.sh" "${prog[@]}" -w "$wt" -S "$slot" -o "$out" -s "$secs" "${steps[*]}" >/dev/null 2>&1
unset IFS
for f in "$run"/smoke_con_*; do [[ -f $f ]] && mv "$f" "$out/"; done
cp "$mygames/FalloutPrefs.ini" "$out/FalloutPrefs.run1.ini" 2>/dev/null
# the restart: the first run's My Games kept (its FalloutPrefs.ini), the setting read back at the main menu, by the
# console and on the Settings menu's Gameplay page (the main menu: Continue, New, Load, Settings, ...)
if wanted settings; then
	mkdir -p "$out/restart"
	restart=("shot:restart_menu" "con:scof+smoke_con_restart;bat+smoke_settings" "sleep:1")
	press restart Up 4
	press restart Down 3
	restart+=("hold:Return:0.3" "sleep:2")
	press restart Up 4
	restart+=("hold:Return:0.3" "sleep:2" "shot:restart_settings" "hold:Escape:0.4" "sleep:2" "hold:Escape:0.4"
		"sleep:2" "cn:qqq" "sleep:15")
	IFS='|'
	"$here/play.sh" "${prog[@]}" -k -w "$wt" -S "$slot" -o "$out/restart" -s 180 "${restart[*]}" >/dev/null 2>&1
	unset IFS
	for f in "$run"/smoke_con_*; do [[ -f $f ]] && mv "$f" "$out/restart/"; done
fi
check=()
[[ $long == 1 ]] && check+=(--long --skip "$skip")
/usr/bin/python3 "$here/play/smoke_check.py" "${check[@]}" "$out" "$mygames/Saves"
