#!/bin/bash


echo -e "Usage : simply choose the board and app\n"

# Display the available board options
echo "Choose board:"
echo "1. x4 "     # npcx_v2 cpu and fiu
echo "2. x7 "    # using npcx cpu and fiu
echo "3. x9 "     # using npcx cpu and fiu
echo "4. k3 "

read -p "Enter your choice (1/2/3/4): " board_choice

# Validate the user's board choice
case $board_choice in
    1) selected_board="npcx4m8f_evb" ;;
    2) selected_board="npcx7m6fb_evb" ;;
    3) selected_board="npcx9m6f_evb" ;;
    4) selected_board="npck3m7k_evb" ;;
    *)
        echo "Invalid choice! Please select a valid option."
        exit 1
        ;;
esac

# Prompt the user for their app name
read -p "Enter your app name: " app_choice

# Function to check if the app folder exists
check_app_exists() {
    if [ ! -d "app/$1" ]; then
        echo "Error: App '$1' does not exist in the 'app' folder."
        exit 1
    fi
}

check_app_exists "$app_choice"


# Set the selected board as an environment variable
export BOARD=$selected_board

# Build and flash the selected app on the chosen board
west build -p always app/$app_choice/
west flash
