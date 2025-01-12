The name comes from Jar + Abelle

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

```bash
java -jar Jarabel-1.0.jar -here
```
- **Only scans the folder where Jarabel is, for example, you could put Jarabel in a mods folder to just inspect the mods**

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

> If the anti-virus pops up it is because of the dcomcheck executable, delete it and the program will still work, but the DcomLaunch section will not be present, the .exe is not dangerous but it interacts with the process memory.

![image](https://github.com/user-attachments/assets/323862db-a222-429b-8cb9-5139b58a5380)
![image](https://github.com/user-attachments/assets/f8ba870b-ee23-4221-84df-2a3ecbe5587d)
![image](https://github.com/user-attachments/assets/9235d72f-3bae-4d39-93e2-b48bc05e1076)
