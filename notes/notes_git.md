# Notes utilisation Git

## Daily Workflow

1. Start the day - get latest changes

```bash
git checkout main       # switch to main branch
git pull origin main    # pull everyting from the main branch. 'origin' is convention name for current remote repository / server (ex: git@github.com:gasLB/example.git). 
```

2. Create / switch to your feature branch

```bash
# DON'T FORGET TO PUSH YOUR BRANCH TO HAVE AN ACCESS ANYWHERE
git checkout -b feature_branch  # -b creates the branch if it doesn't exist yet
```

3. Work on your code and commit frequently

```bash
git add .
git commit -m "description"
```

4. Before pushing, sync with main to avoid conflicts

```bash
git checkout main
git pull origin main
git checkout feature_branch
git rebase main             # put all changes on the latest main before commits of feature_branch. feature_branch is kept up-to-date and compatible with main
```

5. Push your branch

```bash
git push origin feature_branch
```

## Merging back to main

Only when the feature is complete: main should always be functional

```bash
git checkout main
git pull origin main
git merge feature-branch # combine the branches and create a merge commit (3 way or fast-forward)
git push origin main
git branch -d NAME_OF_BRANCH (git branch -D NAME_OF_BRANCH : forces git to delete)
```

## git rebase vs git merge

Before:

```
main:    A---B---C
your-branch:  \---D---E
```

After rebase: 

```
main:    A---B---C
your-branch:      \---D'---E'
```

Or after merge:

```
main:    A---B---C-------F (merge commit)
your-branch:  \---D---E-/
```

-> Do not rebase commits that exist outside your repository and that people may have based work on.

## How to avoid conflicts

- Use features branches
- Rebase before pushing
- Commit small and often
- Don't work on the same file
- git status to see conflicting files

## Tips

git commit -am "description" # automatically adds everything
git revert b234bn23io2  # undo
git stash save coolstuff # save latest changes to a hidden place with name coolstuff
git pop # release saved changes
or
git stash list -> git stash apply ...
git log --graph --oneline --decorate
git bisect # use binary search to search for the good commit 
