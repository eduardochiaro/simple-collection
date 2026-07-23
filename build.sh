#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

EDITIONS=("askew" "binary" "eclipse" "enough" "hollow" "trio")
SPINNER='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
NUM=${#EDITIONS[@]}

PIDS=()
LOGS=()
DONE=()

echo ""
echo -e "${BOLD}${CYAN}  ★  Simple Collection Build${NC}"
echo -e "${DIM}  ─────────────────────────${NC}"
echo ""

# Print placeholder lines
for i in $(seq 0 $((NUM - 1))); do
  echo ""
done

# Start all builds in background
for i in $(seq 0 $((NUM - 1))); do
  log=$(mktemp)
  LOGS[$i]="$log"
  DONE[$i]=""
  (cd "${EDITIONS[$i]}" && pebble clean && pebble build) > "$log" 2>&1 &
  PIDS[$i]=$!
done

spin_idx=0
while true; do
  all_done=true

  for i in $(seq 0 $((NUM - 1))); do
    if [[ -z "${DONE[$i]}" ]]; then
      if kill -0 "${PIDS[$i]}" 2>/dev/null; then
        all_done=false
      else
        wait "${PIDS[$i]}" 2>/dev/null
        if [[ $? -eq 0 ]]; then
          DONE[$i]="pass"
        else
          DONE[$i]="fail"
        fi
      fi
    fi
  done

  # Move cursor up to redraw
  printf "\033[${NUM}A"

  for i in $(seq 0 $((NUM - 1))); do
    edition="${EDITIONS[$i]}"
    printf "\033[2K"  # clear line
    case "${DONE[$i]}" in
      "")
        c="${SPINNER:$spin_idx:1}"
        printf "  ${YELLOW}%s${NC}  ${BOLD}%s${NC} ${DIM}building...${NC}\n" "$c" "$edition"
        ;;
      pass)
        elapsed=$(grep -o "'build' finished successfully.*" "${LOGS[$i]}" 2>/dev/null | head -1 || true)
        extra=""
        if [[ -n "$elapsed" ]]; then
          extra=" ${DIM}│ ${elapsed}${NC}"
        fi
        printf "  ${GREEN}✓${NC}  ${BOLD}%s${NC} ${GREEN}built${NC}%b\n" "$edition" "$extra"
        ;;
      fail)
        printf "  ${RED}✗${NC}  ${BOLD}%s${NC} ${RED}failed${NC}\n" "$edition"
        ;;
    esac
  done

  if $all_done; then
    break
  fi

  spin_idx=$(( (spin_idx + 1) % ${#SPINNER} ))
  sleep 0.08
done

echo ""

# Show errors for any failures
has_failure=false
for i in $(seq 0 $((NUM - 1))); do
  if [[ "${DONE[$i]}" == "fail" ]]; then
    has_failure=true
    echo -e "${RED}${BOLD}  ✗ ${EDITIONS[$i]} errors:${NC}"
    echo -e "${DIM}"
    grep -E "error:|Build failed" "${LOGS[$i]}" | head -15 || true
    echo -e "${NC}"
  fi
done

# Cleanup
for i in $(seq 0 $((NUM - 1))); do
  rm -f "${LOGS[$i]}"
done

if $has_failure; then
  echo -e "${RED}${BOLD}  Build failed.${NC}"
  echo ""
  exit 1
fi

# Collect releases
RELEASES="build-releases"
for i in $(seq 0 $((NUM - 1))); do
  edition="${EDITIONS[$i]}"
  pbw=$(ls "$edition"/build/*.pbw 2>/dev/null | head -1)
  if [[ -z "$pbw" ]]; then
    echo -e "  ${RED}✗${NC}  ${BOLD}${edition}${NC} ${RED}no .pbw found${NC}"
    has_failure=true
    continue
  fi
  version=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['version'])" "$edition/package.json")
  mkdir -p "$RELEASES/$edition"
  cp "$pbw" "$RELEASES/$edition/${edition}-${version}.pbw"
  printf "  ${CYAN}→${NC}  ${DIM}%s/%s/%s-%s.pbw${NC}\n" "$RELEASES" "$edition" "$edition" "$version"
done

echo ""
if $has_failure; then
  echo -e "${RED}${BOLD}  Release copy failed.${NC}"
  echo ""
  exit 1
fi
echo -e "${GREEN}${BOLD}  ★  All watchfaces built successfully!${NC}"
echo ""