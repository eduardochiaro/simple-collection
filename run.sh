# command like ./run.sh chronomark emulator basalt

if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
    echo "Usage: $0 <type> <command_name> <platform>"
    exit 1
fi

TYPE=$1
COMMAND_NAME=$2
PLATFORM=$3

case $COMMAND_NAME in
  "screenshot")
    ./screenshot.sh "$TYPE" "$PLATFORM"
    ;;
  "build")
    cd "${TYPE}" && npm run build
    ;;
  "clean")
    cd "${TYPE}" && npm run clean
    ;;
  "emulator")
    cd "${TYPE}" && npm run emulator "$PLATFORM"
    ;;
  "phone")
    cd "${TYPE}" && npm run phone "$PLATFORM"
    ;;
  "config")
    cd "${TYPE}" && npm run config "$PLATFORM"
    ;;
  "logs")
    cd "${TYPE}" && npm run logs "$PLATFORM"
    ;;
  "tap")
    cd "${TYPE}" && npm run tap "$PLATFORM"
    ;;
  *)
    echo "Unknown command: $COMMAND_NAME"
    exit 1
    ;;
esac