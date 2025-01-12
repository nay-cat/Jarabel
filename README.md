The name comes from Jar + Abelle

---
> [Jarabel for Java 8](https://github.com/nay-cat/Jarabel/releases/download/1.0.3/Jarabel.1.0.3.rar)
---
> [Jarabel compatible with Java 17+](https://github.com/nay-cat/Jarabel/releases/download/1.0.2/Jarabel.1.0.2.rar)
---

Jarabel helps you locate all `.jar` files (or most of them) on a computer. It also uses various checks to simplify or provide detailed information.

### How to Use

- **Scans only the C drive.**
```bash
java -jar Jarabel-1.0.jar
```
---

- **Scans all available drives.**
```bash
java -jar Jarabel-1.0.jar -a
```
---
- **Only scans the folder where Jarabel is, for example, you could put Jarabel in a mods folder to just inspect the mods**
```bash
java -jar Jarabel-1.0.jar -here
```

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

![image](https://github.com/user-attachments/assets/a05f1a3d-9869-4ac5-9c56-c7198d985a68)
![image](https://github.com/user-attachments/assets/45737449-5250-4c4d-b334-f504530e42c3)
![image](https://github.com/user-attachments/assets/9235d72f-3bae-4d39-93e2-b48bc05e1076)
