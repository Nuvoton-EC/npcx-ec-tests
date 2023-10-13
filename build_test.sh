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
echo "1. x4 "
echo "2. x9 "
echo "3. k3 "

read -p "Enter your choice (1/2/3): " board_choice

# Validate the user's board choice
case $board_choice in
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
    branch_name="npcx_k3_0920"
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
git remote update
git rebase

# Function to check if the app folder exists
check_app_exists() {
    if [ ! -d "app/$1" ]; then
        echo "Error: App '$1' does not exist in the 'app' folder."
        exit 1
    fi
}

if [ "$app_choice" = "all" ]; then
    for driver in "${verify_driver[@]}"; do
        check_app_exists "$driver"
        export BOARD=$selected_board
        west build -p always app/$driver/ >> "$log_file" 2>&1
        if [ $? -eq 0 ]; then
            echo -e "app name: $driver, Build Pass"
        else
            echo -e "app name: $driver, Build Failed"
        fi
    done
else
    check_app_exists "$app_choice"
    export BOARD=$selected_board
    west build -p always app/$app_choice/ >> "$log_file" 2>&1
    if [ $? -eq 0 ]; then
        echo -e "Build Pass"
    else
        echo -e "Build Failed"
    fi
fi

#delete build log
rm -rf $log_file
