package com.nay.app;

import com.formdev.flatlaf.FlatDarkLaf;
import com.nay.Jarabel;
import com.nay.check.*;
import com.nay.manager.JournalManager;
import com.nay.model.JarDetails;
import com.nay.utils.CheckUtils;

import javax.imageio.ImageIO;
import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.stream.Collectors;

import static com.nay.utils.CheckUtils.extractMethods;
import static com.nay.utils.CheckUtils.x;

public class Application {

    public void createAndShowGUI() {
        JFrame frame = new JFrame("Jarabel");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(800, 600);

        try {
            for (UIManager.LookAndFeelInfo info : UIManager.getInstalledLookAndFeels()) {
                if ("Nimbus".equals(info.getName())) {
                    UIManager.setLookAndFeel(info.getClassName());
                    UIManager.setLookAndFeel(new FlatDarkLaf());

                    break;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        Color flatDarkBackground = UIManager.getColor("Panel.background");

        frame.getContentPane().setBackground(flatDarkBackground);

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
        tabbedPane.setBackground(Color.DARK_GRAY);
        tabbedPane.setForeground(Color.WHITE);

        tabbedPane.addTab("Jars", panel(j8(SearchCheck.jarFiles.stream())));
        tabbedPane.addTab("Maven", panel(CheckUtils.mavenList));
        tabbedPane.addTab("Gradle", panel(CheckUtils.gradleList));
        tabbedPane.addTab("Forge", panel(CheckUtils.forgeList));
        tabbedPane.addTab("Fabric", panel(CheckUtils.fabricList));
        tabbedPane.addTab("MCP", panel(CheckUtils.mcpList));
        tabbedPane.addTab("Libs", panel(CheckUtils.knownLibs));
        tabbedPane.addTab("Prefetch", panel(PrefetchCheck.prefetchJars));
        tabbedPane.addTab("DcomLaunch", panel(DcomCheck.jarFiles));
        tabbedPane.addTab("Recents", createPanelForListString(RecentCheck.recentPathList));
        tabbedPane.addTab("Runnable Jars", panel(CheckUtils.runnableJars));
        if (Jarabel.journal){
            tabbedPane.addTab("Journal", createPanelForListString(JournalManager.getProcessJarRecords(JournalCheck.jarRecordsList)));
        }

        JLabel imageLabel = new JLabel(new ImageIcon(icon.getImage().getScaledInstance(64, 64, Image.SCALE_SMOOTH)));
        JLabel textLabel = new JLabel("Created by github.com/nay-cat @fluctua", JLabel.CENTER);
        textLabel.setFont(new Font("Arial", Font.PLAIN, 12));

        frame.add(imageLabel, BorderLayout.NORTH);
        frame.add(textLabel, BorderLayout.SOUTH);
        frame.add(tabbedPane, BorderLayout.CENTER);
        frame.setVisible(true);
    }

    private static <T> List<T> j8(java.util.stream.Stream<T> stream) {
        return stream.collect(Collectors.toList());
    }

    private JPanel panel(List<Path> jarList) {
        JPanel panel = new JPanel(new BorderLayout());

        DefaultListModel<JarDetails> jarDetailsModel = new DefaultListModel<>();
        jarList.forEach(path -> {
            File file = path.toFile();
            jarDetailsModel.addElement(new JarDetails(path, file.length(), file.lastModified()));
        });

        JList<JarDetails> jarDetailsList = new JList<>(jarDetailsModel);
        jarDetailsList.setCellRenderer(new Render());
        Render.enableContextMenu(jarDetailsList);

        JScrollPane scrollPane = new JScrollPane(jarDetailsList);
        panel.add(scrollPane, BorderLayout.CENTER);

        JPanel topPanel = new JPanel(new BorderLayout());

        JTextField searchField = new JTextField();
        JComboBox<String> sortComboBox = new JComboBox<>(new String[]{
                "Default", "Size (Ascending)", "Size (Descending)",
                "Last Used (Ascending)", "Last Used (Descending)"
        });
        JComboBox<String> filterComboBox = new JComboBox<>(new String[]{"File size", "Obfuscation degree"});

        JPanel searchAndFilterPanel = new JPanel(new BorderLayout());
        searchAndFilterPanel.add(searchField, BorderLayout.CENTER);
        searchAndFilterPanel.add(sortComboBox, BorderLayout.EAST);
        searchAndFilterPanel.add(filterComboBox, BorderLayout.WEST);

        topPanel.add(searchAndFilterPanel, BorderLayout.CENTER);
        panel.add(topPanel, BorderLayout.NORTH);

        JPanel buttonPanel = new JPanel(new FlowLayout());
        JButton analyzeButton = new JButton("Analyze Selected JAR");
        JButton searchMethodsButton = new JButton("Search classes/methods");
        buttonPanel.add(analyzeButton);
        buttonPanel.add(searchMethodsButton);
        panel.add(buttonPanel, BorderLayout.SOUTH);

        searchField.addKeyListener(new KeyAdapter() {
            @Override
            public void keyReleased(KeyEvent e) {
                String query = searchField.getText().toLowerCase();
                DefaultListModel<JarDetails> filteredModel = new DefaultListModel<>();

                jarList.stream()
                        .filter(path -> path.getFileName().toString().toLowerCase().contains(query))
                        .forEach(path -> {
                            File file = path.toFile();
                            filteredModel.addElement(new JarDetails(path, file.length(), file.lastModified()));
                        });

                jarDetailsList.setModel(filteredModel);
            }
        });

        sortComboBox.addActionListener(e -> {
            String sortOrder = (String) sortComboBox.getSelectedItem();
            List<JarDetails> sortedList = new ArrayList<>();

            for (int i = 0; i < jarDetailsModel.size(); i++) {
                sortedList.add(jarDetailsModel.get(i));
            }

            switch (sortOrder) {
                case "Size (Ascending)":
                    sortedList.sort(Comparator.comparingLong(JarDetails::getSize));
                    break;
                case "Size (Descending)":
                    sortedList.sort(Comparator.comparingLong(JarDetails::getSize).reversed());
                    break;
                case "Last Used (Ascending)":
                    sortedList.sort(Comparator.comparingLong(JarDetails::getLastModified));
                    break;
                case "Last Used (Descending)":
                    sortedList.sort(Comparator.comparingLong(JarDetails::getLastModified).reversed());
                    break;
            }

            DefaultListModel<JarDetails> sortedModel = new DefaultListModel<>();
            sortedList.forEach(sortedModel::addElement);
            jarDetailsList.setModel(sortedModel);
        });

        filterComboBox.addActionListener(e -> {
            String selectedFilter = (String) filterComboBox.getSelectedItem();
            if ("File size".equals(selectedFilter)) {
                applyFileSizeFilter(jarList, jarDetailsModel);
            } else if ("Obfuscation degree".equals(selectedFilter)) {
                applyObfuscationDegreeFilter(jarList, jarDetailsModel);
            }
        });

        analyzeButton.addActionListener(e -> {
            JarDetails selectedJar = jarDetailsList.getSelectedValue();
            if (selectedJar != null) {
                analyzeJar(selectedJar.getPath());
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

                JFrame searchFrame = new JFrame("Search Classes");
                searchFrame.setSize(600, 400);

                JPanel searchPanel = new JPanel(new BorderLayout());
                JTextField methodSearchField = new JTextField();
                DefaultListModel<String> methodsModel = new DefaultListModel<>();
                allMethods.forEach(methodsModel::addElement);
                JList<String> methodsList = new JList<>(methodsModel);
                JScrollPane methodsScrollPane = new JScrollPane(methodsList);

                searchPanel.add(methodSearchField, BorderLayout.NORTH);
                searchPanel.add(methodsScrollPane, BorderLayout.CENTER);

                methodSearchField.addActionListener(event -> performSearch(methodSearchField, methodsModel, methodsList, searchFrame));
                JButton searchButton = new JButton("Search");
                searchPanel.add(searchButton, BorderLayout.SOUTH);

                searchButton.addActionListener(event -> performSearch(methodSearchField, methodsModel, methodsList, searchFrame));
                searchFrame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

                searchFrame.addWindowListener(new java.awt.event.WindowAdapter() {
                    @Override
                    public void windowClosing(java.awt.event.WindowEvent windowEvent) {
                        allMethods.clear();
                        methodsModel.clear();
                        methodsList.setModel(new DefaultListModel<>());

                        searchFrame.dispose();

                        // esto esuna oración no una orden
                        System.gc();
                        System.runFinalization();
                    }
                });

                searchFrame.add(searchPanel);
                searchFrame.setVisible(true);
            });
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

    private void performSearch(JTextField searchField, DefaultListModel<String> methodsModel, JList<String> methodsList, JFrame searchFrame) {
        String query = searchField.getText().toLowerCase(); // Obtener la búsqueda
        if (!searchField.getText().isEmpty()) {
            DefaultListModel<String> filteredMethodsModel = new DefaultListModel<>();
            for (int i = 0; i < methodsModel.getSize(); i++) {
                String method = methodsModel.get(i);
                if (method.toLowerCase().contains(query)) {
                    // meter los colores, debo quitar los <br> cuando pueda
                    filteredMethodsModel.addElement("<html>" + method.replaceAll("(?i)(" + query + ")", "<span style='background:yellow;'>$1</span>") + "</html>");
                }
            }

            methodsList.setModel(filteredMethodsModel);

            if (!filteredMethodsModel.isEmpty()) {
                methodsList.setSelectedIndex(0);
                methodsList.ensureIndexIsVisible(0);
            } else {
                JOptionPane.showMessageDialog(searchFrame, "No classes founds " + query);
            }
        } else {
            JOptionPane.showMessageDialog(searchFrame, "Inupt error");
        }
    }

    private void applyFileSizeFilter(List<Path> jarList, DefaultListModel<JarDetails> listModel) {
        listModel.clear();
        for (Path jar : jarList) {
            try {
                long fileSizeInBytes = Files.size(jar);
                // 8.388.608
                if (fileSizeInBytes < 8 * 1024 * 1024) {
                    File file = jar.toFile();
                    listModel.addElement(new JarDetails(jar, fileSizeInBytes, file.lastModified()));
                }
            } catch (IOException e) {
                System.err.println("Error reading file size: " + jar + " - " + e.getMessage());
            }
        }
    }

    private void applyObfuscationDegreeFilter(List<Path> jarList, DefaultListModel<JarDetails> listModel) {
        listModel.clear();
        for (Path jar : jarList) {
            try (JarFile jarFile = new JarFile(jar.toFile())) {
                // 16.777.216
                if (Files.size(jar) < 16 * 1024 * 1024) {
                    List<String> methods = extractMethods(jarFile);
                    double obfuscationDegree = x(methods);
                    if (obfuscationDegree >= 3.1 && obfuscationDegree <= 3.5) {
                        File file = jar.toFile();
                        listModel.addElement(new JarDetails(jar, file.length(), file.lastModified()));
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

}
