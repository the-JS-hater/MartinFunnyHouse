import subprocess
import argparse


def run_git_commands(commit_message):
    """
    Automates the process of typing out:

    git add -A && git commit -m "some message" && git pull --rebase && git push

    Usage: python3 git-commit.py -"some message"
    
    """
    try:
        # Stage all changes
        subprocess.run(['git', 'add', '-A'], check=True)
        
        # Commit changes
        subprocess.run(['git', 'commit', '-m', commit_message], check=True)
        
        # Pull with rebase
        subprocess.run(['git', 'pull', '--rebase'], check=True)
        
        # Push changes
        subprocess.run(['git', 'push'], check=True)
        
        print("Git commit executed successfully.")
        
    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Automate Git commands.')
    parser.add_argument('message', type=str, help='Commit message for the git commit')
    
    args = parser.parse_args()
    
    run_git_commands(args.message)
