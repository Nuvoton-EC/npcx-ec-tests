#!/bin/bash

log_file="$(pwd)/build_log.txt"
echo -n > "$log_file"

verify_driver=()

for dir in ~/zephyrproject/npcx_tests/app/*/; do
  dir_name=$(basename "$dir")

  if [ "$dir_name" != "build" ]; then
    verify_driver+=("$dir_name")
  fi
done

echo -e "Usage : simply choose the board and app\n"

# Display the available board options
echo "Choose board:"
echo "0. choice zephyr repo branch name and board name:"
echo "1. x4 "
echo "2. x9 "
echo "3. k3 "

read -p "Enter your choice (0/1/2/3): " board_choice

# Validate the user's board choice
case $board_choice in
	0)
	read -p "Enter zephyr branch name: " branch_name
	read -p "Enter npcx_tests board : " selected_board
	;;
    1)
    selected_board="npcx4m8f_evb"
    branch_name="main"
    ;;
    2)
    selected_board="npcx9m6f_evb"
    branch_name="main"
    ;;
    3)
    selected_board="npck3m7k_evb"
    branch_name="npcx_k3_1025"
    ;;
    *)
        echo "Invalid choice! Please select a valid option."
        exit 1
        ;;
esac

# Prompt the user for their app name
echo -e "notice: build all app, please key in all."
read -p "Enter your app name: " app_choice
echo -e "app: $app_choice, board: $selected_board"

#remote update zephyr
cd ~/zephyrproject/zephyr/
git remote update
git checkout $branch_name
git rebase --onto $branch_name HEAD
git pull

#remote update npcx_tesst
cd ~/zephyrproject/npcx_tests/
git pull

#Activate the virtual environment:
source ~/zephyrproject/.venv/bin/activate

# Function to check if the app folder exists
check_app_exists() {
    if [ ! -d "app/$1" ]; then
        echo "Error: App '$1' does not exist in the 'app' folder."
        exit 1
    fi
}

check_overlay_file() {
    if [ -f "app/$1/boards/$selected_board.overlay" ]; then
        export BOARD=$selected_board
        west build -p always app/$1/ >> "$log_file" 2>&1
        if [ $? -eq 0 ]; then
            echo -e "app name: $1, Build Pass"
        else
            echo -e "app name: $1, Build Failed"
        fi
    else
        echo "Skipping build for app '$1' as overlay file '$selected_board.overlay' does not exist."
    fi
}

if [ "$app_choice" = "all" ]; then
    for driver in "${verify_driver[@]}"; do
        check_app_exists "$driver"
		check_overlay_file "$driver"
    done
else
    check_app_exists "$app_choice"
    check_overlay_file "$app_choice"
fi


#delete build log
rm -rf $log_file
