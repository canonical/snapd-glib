# Contributing to snapd-glib

We are an open source project and welcome community contributions, suggestions,
fixes and constructive feedback.

If you'd like to contribute, you will first need to sign the Canonical
contributor agreement. This is the easiest way for you to give us permission to
use your contributions. In effect, you’re giving us a license, but you still
own the copyright — so you retain the right to modify your code and use it in
other projects.

The agreement can be found, and signed, here:
https://ubuntu.com/legal/contributors

## Contributor guidelines

Contributors can help us by observing the following guidelines:

- Commit messages should be well structured.
- Commit emails should not include non-ASCII characters.
- Several smaller PRs are better than one large PR.
- Try not to mix potentially controversial and trivial changes together.
  (Proposing trivial changes separately makes landing them easier and
  makes reviewing controversial changes simpler)
- Do not [force push][git-force] a PR after it has received reviews. It is
  acceptable to force push when a PR is ready to merge, however.
- Try to write tests to cover the contributed changes

## Pull request guidelines

Contributions are submitted through a [pull request][pull-request] created from
a [fork][fork] of the `snapd` repository (under your GitHub account).

GitHub's documentation outlines the [process][github-pr], but for a more
concise and informative version try [this GitHub gist][pr-gist].

### Linear git history

We strive to keep a [linear git history][linear-git]. This makes it easier to
inspect the history, keep related commits next to each other, and make tools
like [git bisect][git-bisect] work intuitively.

### Pull request updates

Feel free to [rebase][github-rebase], rework commits, and [force
push][git-force] to your branch while a PR is waiting for its first review.

However, if you are still making significant changes during this waiting
phase, it's a good idea to keep the PR as a [draft][github-draft]. This stops
reviewers from looking at code you may not be confident about. Set the PR as
"Ready for review" when you do feel confident.

During the review process, reviewers will point out defects or suggest
alternative implementations.

After the first review, please treat your already pushed commits as immutable
and submit any requested changes as additional commits. This helps reviewers to
see exactly what has changed since the last review without requesting them to
review all the changes.

After approval, you can rework the branch history as you see fit. Consider
squashing commits from the original PR with those made during the review
process, for example. Commit messages should follow the format described in
[CODING.md](CODING.md). A [force push][git-force] will be required if you
rework the history.

Start a [rebase][github-rebase] from the original parent commit of your first
commit. Ensure you do not rebase on top of the current main as this means
changes from the _main_ branch will be shown in the GitHub UI as part of your
changes, making the verification more confusing.

Merge using Github's [Squash and Merge][github-squash-merge] or [Rebase and merge][github-rebase-merge],
never [Create a merge commit][github-merge-commit].
* [Squash and Merge][github-squash-merge] is preferred because it simplifies cherry-picking of PR
content.
  * Also for single commits
  * This merge will use the title as commit message so double check that it is accurate and concise
* [Rebase and merge][github-rebase-merge] is required when it is important to be able to distinguish
different parts of a solution in the future.
  * Keep commits to a minimum
  * Squash uninteresting commits such as review improvements after review approval
