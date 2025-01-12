

> **Jarabel** is a program still under development and currently works only with **Java 17**.

Jarabel helps you locate all `.jar` files (or most of them) on a computer. It also uses various checks to simplify or provide detailed information.

### How to Use

```bash
java -jar Jarabel-1.0.jar
```
- **Scans only the C drive.**

```bash
java -jar Jarabel-1.0.jar -a
```
- **Scans all available drives.**

### Features

| **Feature**                                        | **Description**                                                                                          |
|----------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| **Categorization by type**                         | Jarabel distinguishes between Maven, Gradle, Forge, Fabric, and others.                                 |                                         |
| **Class inspection**                               | Inspect classes from all `.jar` files in a single search.               |
| **Obfuscation percentage analysis**                | Provides an obfuscation percentage for each `.jar`.                                                     |
| **Library information**                            | Shows detailed information about libraries                                   |
| **Additional memory and registry checks**          | Includes extra checks for memory, registry (regedit), and prefetch information.                         |
| **Coming soon**                                    | More features are on the way (maybe)                                                                           |

---

### Notes

> The code is still **under development**, and optimization is needed. Performance may vary depending on the user's environment and computer specifications.


