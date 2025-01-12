package com.nay.app;

import com.nay.check.DcomCheck;
import com.nay.check.PrefetchCheck;
import com.nay.check.RecentCheck;
import com.nay.check.SearchCheck;
import com.nay.utils.CheckUtils;

import javax.imageio.ImageIO;
import javax.swing.*;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

import static com.nay.utils.CheckUtils.x;

public class Application {

    public void createAndShowGUI() {
        JFrame frame = new JFrame("Jarabell");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(800, 600);

        ImageIcon icon = null;

        try (InputStream imageStream = getClass().getResourceAsStream("/art.jpg")) {
            if (imageStream != null) {
                BufferedImage bufferedImage = ImageIO.read(imageStream);
                icon = new ImageIcon(bufferedImage);
            } else {
                System.err.println("Image not found in resources!");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        frame.setIconImage(icon.getImage());

        JTabbedPane tabbedPane = new JTabbedPane();

        tabbedPane.addTab("Jars", createPanelForList(SearchCheck.jarFiles.stream().toList()));
        tabbedPane.addTab("Maven", createPanelForList(CheckUtils.mavenList));
        tabbedPane.addTab("Gradle", createPanelForList(CheckUtils.gradleList));
        tabbedPane.addTab("Forge", createPanelForList(CheckUtils.forgeList));
        tabbedPane.addTab("Fabric", createPanelForList(CheckUtils.fabricList));
        tabbedPane.addTab("MCP", createPanelForList(CheckUtils.mcpList));
        tabbedPane.addTab("Libs", createPanelForList(CheckUtils.knownLibs));
        tabbedPane.addTab("Prefetch", createPanelForList(PrefetchCheck.prefetchJars));
        tabbedPane.addTab("DcomLaunch", createPanelForList(DcomCheck.jarFiles));
        tabbedPane.addTab("Recents", createPanelForListString(RecentCheck.recentPathList));

        JLabel imageLabel = new JLabel(new ImageIcon(icon.getImage().getScaledInstance(64, 64, Image.SCALE_SMOOTH)));
        JLabel textLabel = new JLabel("Created by github.com/nay-cat @fluctua", JLabel.CENTER);
        textLabel.setFont(new Font("Arial", Font.PLAIN, 12));

        frame.add(imageLabel, BorderLayout.NORTH);
        frame.add(textLabel, BorderLayout.SOUTH);

        frame.add(tabbedPane, BorderLayout.CENTER);
        frame.setVisible(true);
    }

    private JPanel createPanelForList(List<Path> jarList) {
        JPanel panel = new JPanel(new BorderLayout());

        DefaultListModel<Path> listModel = new DefaultListModel<>();
        jarList.forEach(listModel::addElement);
        JList<Path> list = new JList<>(listModel);
        list.setCellRenderer(new CheckUtils.CustomCellRenderer());
        JScrollPane scrollPane = new JScrollPane(list);
        panel.add(scrollPane, BorderLayout.CENTER);

        JPanel topPanel = new JPanel(new FlowLayout());
        JLabel filterLabel = new JLabel("Filter by:");
        JComboBox<String> filterComboBox = new JComboBox<>(new String[]{"File size", "Obfuscation degree"});
        topPanel.add(filterLabel);
        topPanel.add(filterComboBox);
        panel.add(topPanel, BorderLayout.NORTH);

        JPanel buttonPanel = new JPanel(new FlowLayout());
        JButton analyzeButton = new JButton("Analyze Selected JAR");
        JButton searchMethodsButton = new JButton("Search classes/methods");
        buttonPanel.add(analyzeButton);
        buttonPanel.add(searchMethodsButton);

        panel.add(buttonPanel, BorderLayout.SOUTH);

        analyzeButton.addActionListener(e -> {
            Path selectedJar = list.getSelectedValue();
            if (selectedJar != null) {
                analyzeJar(selectedJar);
            } else {
                JOptionPane.showMessageDialog(panel, "No JAR selected!", "Error", JOptionPane.ERROR_MESSAGE);
            }
        });

        searchMethodsButton.addActionListener(e -> {
            SwingUtilities.invokeLater(() -> {
                List<String> allMethods = new ArrayList<>();
                for (Path jar : jarList) {
                    try (JarFile jarFile = new JarFile(jar.toFile())) {
                        allMethods.addAll(extractMethods(jarFile));
                    } catch (IOException ex) {
                        System.err.println("Error reading classes from JAR: " + jar + " - " + ex.getMessage());
                    }
                }

                JFrame searchFrame = new JFrame("Search Methods");
                searchFrame.setSize(600, 400);

                JPanel searchPanel = new JPanel(new BorderLayout());
                JTextField searchField = new JTextField();
                DefaultListModel<String> methodsModel = new DefaultListModel<>();
                allMethods.forEach(methodsModel::addElement);
                JList<String> methodsList = new JList<>(methodsModel);
                JScrollPane methodsScrollPane = new JScrollPane(methodsList);

                searchPanel.add(searchField, BorderLayout.NORTH);
                searchPanel.add(methodsScrollPane, BorderLayout.CENTER);

                JButton searchButton = new JButton("Search");
                searchPanel.add(searchButton, BorderLayout.SOUTH);

                searchButton.addActionListener(event -> {
                    String query = searchField.getText().toLowerCase(); // Obtener el texto de búsqueda
                    if (!query.isEmpty()) {
                        DefaultListModel<String> filteredMethodsModel = new DefaultListModel<>();

                        for (int i = 0; i < methodsModel.getSize(); i++) {
                            String method = methodsModel.get(i);
                            if (method.toLowerCase().contains(query)) {
                                filteredMethodsModel.addElement(method);
                            }
                        }

                        methodsList.setModel(filteredMethodsModel);

                        if (!filteredMethodsModel.isEmpty()) {
                            methodsList.setSelectedIndex(0);
                            methodsList.ensureIndexIsVisible(0);
                        } else {
                            JOptionPane.showMessageDialog(searchFrame, "No methods found matching: " + query, "Search Results", JOptionPane.INFORMATION_MESSAGE);
                        }
                    } else {
                        JOptionPane.showMessageDialog(searchFrame, "Please enter a method name to search.", "Input Error", JOptionPane.ERROR_MESSAGE);
                    }
                });

                searchFrame.add(searchPanel);
                searchFrame.setVisible(true);
            });
        });

        filterComboBox.addActionListener(e -> {
            String selectedFilter = (String) filterComboBox.getSelectedItem();
            if ("File size".equals(selectedFilter)) {
                applyFileSizeFilter(jarList, listModel);
            } else if ("Obfuscation degree".equals(selectedFilter)) {
                applyObfuscationDegreeFilter(jarList, listModel);
            }
        });

        return panel;
    }

    private JPanel createPanelForListString(List<String> stringList) {
        JPanel panel = new JPanel(new BorderLayout());

        DefaultListModel<String> listModel = new DefaultListModel<>();
        stringList.forEach(listModel::addElement);

        JList<String> list = new JList<>(listModel);
        JScrollPane scrollPane = new JScrollPane(list);
        panel.add(scrollPane, BorderLayout.CENTER);

        JPanel topPanel = new JPanel(new FlowLayout());
        JLabel titleLabel = new JLabel("Items in the List:");
        topPanel.add(titleLabel);
        panel.add(topPanel, BorderLayout.NORTH);

        return panel;
    }

    private void applyFileSizeFilter(List<Path> jarList, DefaultListModel<Path> listModel) {
        listModel.clear();
        for (Path jar : jarList) {
            try {
                long fileSizeInBytes = Files.size(jar);
                if (fileSizeInBytes < 8 * 1024 * 1024) {
                    listModel.addElement(jar);
                }
            } catch (IOException e) {
                System.err.println("Error reading file size: " + jar + " - " + e.getMessage());
            }
        }
    }

    private void applyObfuscationDegreeFilter(List<Path> jarList, DefaultListModel<Path> listModel) {
        listModel.clear();
        for (Path jar : jarList) {
            try (JarFile jarFile = new JarFile(jar.toFile())) {
                if (Files.size(jar) < 16 * 1024 * 1024) {
                    List<String> methods = extractMethods(jarFile);
                    double obfuscationDegree = x(methods);
                    if (obfuscationDegree >= 3.1 && obfuscationDegree <= 3.5) {
                        listModel.addElement(jar);
                    }
                }
            } catch (IOException e) {
                System.err.println("Error analyzing obfuscation degree: " + jar + " - " + e.getMessage());
            }
        }
    }

    private void analyzeJar(Path jarPath) {
        try (JarFile jarFile = new JarFile(jarPath.toFile())) {
            List<String> methods = extractMethods(jarFile);
            double obfuscationDegree = x(methods);

            JFrame resultFrame = new JFrame("Analysis Results: " + jarPath.getFileName());
            resultFrame.setSize(600, 400);

            JTabbedPane resultTabs = new JTabbedPane();

            resultTabs.addTab("Methods", new JScrollPane(new JTextArea(String.join("\n", methods))));
            resultTabs.addTab("Obfuscation", new JLabel("Obfuscation Degree: " + obfuscationDegree));

            resultFrame.add(resultTabs);
            resultFrame.setVisible(true);
        } catch (IOException e) {
            JOptionPane.showMessageDialog(null, "Failed to analyze JAR: " + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private List<String> extractMethods(JarFile jarFile) throws IOException {
        List<String> methods = new ArrayList<>();
        Enumeration<JarEntry> entries = jarFile.entries();

        while (entries.hasMoreElements()) {
            JarEntry entry = entries.nextElement();
            if (entry.getName().endsWith(".class")) {
                methods.add(entry.getName());
            }
        }
        return methods;
    }

}
