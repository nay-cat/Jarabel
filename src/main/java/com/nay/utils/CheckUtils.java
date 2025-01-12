package com.nay.utils;

import javax.swing.*;
import java.awt.*;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.*;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.stream.Collectors;

import static com.nay.check.SearchCheck.*;

public class CheckUtils {

    // static static static clap clap clap
    public static final List<Path> mavenList = Collections.synchronizedList(new ArrayList<>());
    public static final List<Path> gradleList = Collections.synchronizedList(new ArrayList<>());
    public static final List<Path> forgeList = Collections.synchronizedList(new ArrayList<>());
    public static final List<Path> fabricList = Collections.synchronizedList(new ArrayList<>());
    public static final List<Path> mcpList = Collections.synchronizedList(new ArrayList<>());
    public static final List<Path> knownLibs = Collections.synchronizedList(new ArrayList<>());

    public static void collectJarFiles(Path rootDirectory) {
        try {
            Files.walkFileTree(rootDirectory, new SimpleFileVisitor<>() {
                @Override
                public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) {
                    if (file.toString().endsWith(".jar")) {
                        jarFiles.add(file);
                        System.out.println("Found JAR: " + file);
                    }
                    return FileVisitResult.CONTINUE;
                }

                @Override
                public FileVisitResult visitFileFailed(Path file, IOException exc) {
                    System.err.println("Cannot access: " + file + " -> " + exc.getMessage());
                    return FileVisitResult.SKIP_SUBTREE;
                }
            });
        } catch (IOException e) {
            System.err.println("Error walking file tree: " + e.getMessage());
        }
    }

    public static void analyzeJarsInParallel() {
        List<Callable<Void>> tasks = new ArrayList<>();
        for (Path jar : jarFiles) {
            tasks.add(() -> {
                try {
                    analyzeJar(jar);
                } catch (Exception e) {
                    System.err.println("Error analyzing JAR: " + jar + " -> " + e.getMessage());
                }
                return null;
            });
        }

        try {
            pool.invokeAll(tasks);
        } catch (Exception e) {
            System.err.println("Task interrupted: " + e.getMessage());
            Thread.currentThread().interrupt();
        } finally {
            pool.shutdown();
        }
    }

    private static void analyzeJar(Path jar) {
        try (JarFile jarFile = new JarFile(jar.toFile())) {
            boolean isMaven = jarFile.stream().anyMatch(e -> e.getName().contains("pom.xml"));
            boolean isGradle = jarFile.stream().anyMatch(e -> e.getName().contains("build.gradle"));
            boolean hasForge = jarFile.getEntry("mcmod.info") != null || jarFile.stream().anyMatch(e -> e.getName().contains("net.minecraftforge.fml"));
            boolean hasFabric = jarFile.getEntry("fabric.mod.json") != null;

            boolean hasMcp = jarFile.stream()
                    .filter(e -> e.getName().endsWith(".class"))
                    .limit(10)
                    .anyMatch(e -> containsStrings(jarFile, e, "func_", "field_", "mc"));

            if (isMaven) mavenList.add(jar);
            if (isGradle) gradleList.add(jar);
            if (hasForge) forgeList.add(jar);
            if (hasFabric) fabricList.add(jar);
            if (hasMcp) mcpList.add(jar);

            boolean hasNativeHook = false;
            boolean hasInputEvent = false;

            for (JarEntry entry : Collections.list(jarFile.entries())) {
                if (entry.getName().endsWith(".class")) {
                    if (by(jarFile, entry, "org/jnativehook/mouse/".getBytes())) {
                        hasNativeHook = true;
                    }
                }
                // check removed bc laggy as fuck
                if (hasNativeHook && hasInputEvent) break;
            }

            if (hasNativeHook || hasInputEvent) {
                knownLibs.add(jar);
            }

            synchronized (System.out) {
                System.out.println("Processed: " + jar +
                        " | Maven: " + isMaven +
                        " | Gradle: " + isGradle +
                        " | Forge: " + hasForge +
                        " | Fabric: " + hasFabric +
                        " | MCP: " + hasMcp +
                        " | NativeHook: " + hasNativeHook +
                        " | InputEvent: " + hasInputEvent);
            }
        } catch (IOException e) {
            System.err.println("Failed to process: " + jar + " -> " + e.getMessage());
        }
    }

    private static boolean containsStrings(JarFile jarFile, JarEntry entry, String... strings) {
        try (InputStream is = jarFile.getInputStream(entry);
             BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
            String content = reader.lines().collect(Collectors.joining("\n"));
            return Arrays.stream(strings).allMatch(content::contains);
        } catch (IOException e) {
            return false;
        }
    }

    private static boolean by(JarFile z, JarEntry x, byte[] y) {
        try (InputStream w = z.getInputStream(x)) {
            byte[] v = new byte[4096];
            int u;
            int t = 0;

            while ((u = w.read(v)) != -1) {
                for (int s = 0; s < u; s++) {
                    if (v[s] == y[t]) {
                        t++;
                        if (t == y.length) {
                            return true;
                        }
                    } else {
                        t = 0;
                    }
                }
            }
        } catch (IOException r) {
            System.err.println("Error reading JAR: " + x.getName() + " -> " + r.getMessage());
        }
        return false;
    }


    public static double x(List<String> a) {
        double b = 0;
        for (String c : a) {
            b += y(c);
        }
        return b / a.size();
    }

    public static double y(String d) {
        int[] e = new int[256];
        for (char f : d.toCharArray()) {
            e[f]++;
        }

        double g = 0;
        for (int h : e) {
            if (h > 0) {
                double i = (double) h / d.length();
                g -= i * (Math.log(i) / Math.log(2));
            }
        }
        return g;
    }


    public static class CustomCellRenderer extends DefaultListCellRenderer {
        @Override
        public Component getListCellRendererComponent(JList<?> list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
            Component component = super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
            if (value instanceof Path) {
                Path path = (Path) value;
                try {
                    long fileSizeInBytes = Files.size(path);
                    if (fileSizeInBytes < 8 * 1024 * 1024) {
                        JarFile jarFile = new JarFile(path.toFile());
                        List<String> methods = new ArrayList<>();
                        Enumeration<JarEntry> entries = jarFile.entries();

                        while (entries.hasMoreElements()) {
                            JarEntry entry = entries.nextElement();
                            if (entry.getName().endsWith(".class")) {
                                methods.add(entry.getName());
                            }
                        }
                        double obfuscationDegree = x(methods);
                        if (obfuscationDegree >= 3.1 && obfuscationDegree <= 3.5) {
                            setForeground(Color.RED);
                        }
                    }
                } catch (IOException e) {
                }
            }
            return component;
        }
    }
}



