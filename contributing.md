# Gpredict Contribution Guidelines #

## Introduction ##

Thank you for considering contributing to Gpredict. Below you can find some
brief guidelines for how to make your first contribution.

The most important thing to understand is that Gpredict is "old" software with
the first release dating back to 2001. As a consequence, it contains a lot of
legacy code and design choices that do not live up to what one would expect
from modern desktop software. Therefore, the primary focus for the time being
is fixing bugs and rewriting the parts that are causing poor user experience
or difficulties in maintaining the software.

Contributions that add new features or functionality not currently on the
roadmap have very low priority. The roadmap consists mostly of, in prioritized
order:

1. Issues included in a [milestone](https://github.com/csete/gpredict/milestones)
2. Issues tagged with the labels
   [bug](https://github.com/csete/gpredict/labels/bug),
   [enhancement](https://github.com/csete/gpredict/labels/enhancement),
   [feature](https://github.com/csete/gpredict/labels/feature),
   [documents](https://github.com/csete/gpredict/labels/feature), and
   [help wanted](https://github.com/csete/gpredict/labels/help%20wanted)

## How Can You Help? ##

The areas where I need most help with at the moment are:

1. Testing and [reporting bugs](#reporting-bugs)
2. [Fixing bugs](#fixing-bugs)
3. [Providing user support](#providing-user-support)
4. [Packaging](#packaging)
5. [Website](#website)

## Reporting Bugs ##

If you have found a new bug in Gpredict, please report it using the
[Gpredict issue tracker](https://github.com/csete/gpredict/issues) on GitHub.
Please make sure to provide all the information that is necessary to reproduce
the bug, including but not limited to:

1. Your operating system.
2. Which version of Gpredict and how it was installed.
3. Steps required to trigger the bug.

## Fixing Bugs ##

Bugfixes are always welcome and are a very good way for making your first
contribution and they make a good first impression. However, before submitting
a pull request, please ensure that:

* Your changes are as small as possible and fix only one bug.
* Your changes are well tested.
* Your commits are atomic, i.e. complete and functional.
* Write [good commit messages](https://chris.beams.io/posts/git-commit/) and
  make sure to reference the issue you are fixing.
* Avoid submitting merge commits; rebase your working branch instead.
* If you have a series of commits where one commit fixes a previous one, squash them.
* The pull request should cover only one topic, otherwise the review will be
  difficult and confusing.

Furthermore, until we get a formal coding style guide, please try to follow the coding
style used in the surrounding code.

## Providing User Support ##

We have a very nice [user support forum](https://community.libre.space/c/gpredict/)
hosted by Libre Space Foundation. Please help others who need technical support.

## Packaging ##

I am doing my best to make Gpredict available on as many platforms as possible.
While I can write platform independent code, I could definitely use some  help
with packaging and testing Gpredict on Windows and Mac OS X.

## Website ##

I think that the [existing website](http://gpredict.oz9aec.net/) makes it very
clear that I need help with creating a new website. Please let me know if you
wish to help.

Thanks for reading this far 🙂

Alex, OZ9AEC
