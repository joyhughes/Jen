
# **Contributing Guidelines** 📄

This documentation contains a set of guidelines to help you during the contribution process.
We are happy to welcome all the contributions from anyone willing to improve/add new scripts to this project.
Thank you for helping out and remember, **no contribution is too small.**
<br>
Please note we have a [code of conduct](CODE_OF_CONDUCT.md)  please follow it in all your interactions with the project.



<br>

## **Need some help regarding the basics?🤔**


You can refer to the following articles on basics of Git and Github and also contact the Project Mentors,
in case you are stuck:

- [Forking a Repo](https://help.github.com/en/github/getting-started-with-github/fork-a-repo)
- [Cloning a Repo](https://help.github.com/en/desktop/contributing-to-projects/creating-an-issue-or-pull-request)
- [How to create a Pull Request](https://opensource.com/article/19/7/create-pull-request-github)
- [Getting started with Git and GitHub](https://towardsdatascience.com/getting-started-with-git-and-github-6fcd0f2d4ac6)
- [Learn GitHub from Scratch](https://docs.github.com/en/get-started/start-your-journey/git-and-github-learning-resources)

<br>

## **Issue Report Process 📌**

1. Go to the project's issues.
2. Give proper description for the issues.
3. Don't spam to get the assignment of the issue 😀.
4. Wait for till someone is looking into it !.
5. Start working on issue only after you got assigned that issue 🚀.

<br>

## **Pull Request Process 🚀**

1. Ensure that you have self reviewed your code 😀
2. Make sure you have added the proper description for the functionality of the code
3. I have commented my code, particularly in hard-to-understand areas.
4. Add screenshot it help in review.
5. Submit your PR by giving the necesarry information in PR template and hang tight we will review it really soon 🚀

<br>

# **Thank you for contributing💗** 
=======


---

## Contributing Guidelines

When contributing to this repository, it is essential to first discuss the proposed changes with the project maintainers via issue to ensure alignment with the project's goals.

Please adhere to our Code of Conduct during all interactions within the project.

### Pull Request Process

1. **Dependency Management**: Before concluding a build, ensure that all installation or build dependencies are removed. Commit only relevant files to maintain repository cleanliness, disregarding the rest.

2. **Update README.md**: Detail any changes to the interface in the README.md file. This includes information such as new environment variables, exposed ports, useful file locations, and container parameters.

3. **Review Request**: Once a Pull Request (PR) is submitted, request a review from the project maintainers to facilitate timely feedback and integration.

### Git Workflow

#### Step 1: Fork Repository

#### Step 2: Git Setup & Repository Download

```bash
# Clone the repository
$ git clone https://github.com/<User-Name>/<Repo-Name>.git

# Add upstream remote
$ git remote add upstream https://github.com/ashutosh-rath02/git-re.git

# Fetch and merge with upstream/master
$ git fetch upstream
$ git merge upstream/master
```

#### Step 3: Create and Publish Working Branch

```bash
$ git checkout -b <type>/<issue|issue-number>/{<additional-fixes>}
$ git push origin <type>/<issue|issue-number>/{<additional-fixes>}
```

**Types:**
- `wip`: Work in Progress; mainstream changes; long-term work.
- `feat`: New Feature; non-mainstream changes; future planned.
- `bug`: Bug Fixes.
- `exp`: Experimental; random experimental features.

#### On Task Completion

```bash
# Ensure on the branch
$ git branch

# Fetch and merge with upstream/master
$ git fetch upstream
$ git merge upstream/master

# Add untracked files
$ git add .

# Commit changes with appropriate message and description
$ git commit -m "your-commit-message" -m "your-commit-description"

# Fetch and merge with upstream/master again
$ git fetch upstream
$ git merge upstream/master

# Push changes to your forked repository
$ git push origin <type>/<issue|issue-number>/{<additional-fixes>}
```

#### Creating the Pull Request (PR) using GitHub Website

- Create a PR from `<type>/<issue|issue-number>/{<additional-fixes>}` branch in your forked repository to the master branch in the upstream repository.
- After creating the PR, add a Reviewer (Any Admin) and yourself as the assignee.
- Link the PR to the appropriate Issue or Project+Milestone if no issue has been created.
- **Important**: Do Not Merge the PR unless specifically instructed by an admin.

### After PR Merge

- Delete the branch from the forked repository.
- Clean up the local environment by updating the master branch.

```bash
$ git branch -d <type>/<issue|issue-number>/{<additional-fixes>}
$ git push --delete origin <type>/<issue|issue-number>/{<additional-fixes>}
$ git checkout master
$ git pull upstream
$ git push origin
```

Always adhere to commit message standards and maintain a consistent style.

---
