package com.nay.app;

import com.nay.model.JarDetails;

import javax.swing.*;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.List;
import java.util.jar.JarFile;
import java.util.zip.ZipException;

import static com.nay.utils.CheckUtils.extractMethods;
import static com.nay.utils.CheckUtils.x;

public class Render extends DefaultListCellRenderer {

    @Override
    public Component getListCellRendererComponent(JList<?> list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
        JLabel label = (JLabel) super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
        if (value instanceof JarDetails) {
            JarDetails jarDetails = (JarDetails) value;
            String fileName = jarDetails.getHighlightedName();
            double sizeInMb = jarDetails.getSize() / (1024.0 * 1024.0);
            String lastUsed = new Date(jarDetails.getLastModified()).toString();
            String path = jarDetails.getPath().toString();

            double obfuscationDegree = calculateObfuscationDegree(jarDetails);

            String obfuscationText = "";
            if (obfuscationDegree >= 3.1 && obfuscationDegree <= 3.5) {
                obfuscationText = " <font color='red'><b>HIGH OBFUSCATION DEGREE DETECTED</b></font>";
            }

            label.setText(String.format(
                    "<html><b>%s</b> - <font color='#6e6e6e'>Size: %.2f MB</font> - <font color='green'>Last Used: %s</font><br><font color='#ffffff'>%s</font>%s</html>",
                    fileName, sizeInMb, lastUsed, path, obfuscationText
            ));
        }
        return label;
    }

    private double calculateObfuscationDegree(JarDetails jarDetails) {
        File file = jarDetails.getPath().toFile();

        if (file.length() == 0) {
            System.err.println("Skipping empty file: " + file.getName());
            return 0;
        }

        try (JarFile jarFile = new JarFile(file)) {
            List<String> methodNames = extractMethods(jarFile);
            return x(methodNames);
        } catch (ZipException e) {
            return 0;
        } catch (IOException e) {
            e.printStackTrace();
            return 0;
        }
    }

    public static void enableContextMenu(JList<JarDetails> list) {
        list.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    int index = list.locationToIndex(e.getPoint());
                    if (index != -1) {
                        JarDetails jarDetails = list.getModel().getElementAt(index);
                        showContextMenu(list, e.getPoint(), jarDetails);
                    }
                }
            }
        });
    }

    private static void showContextMenu(JList<JarDetails> list, Point point, JarDetails jarDetails) {
        JPopupMenu contextMenu = new JPopupMenu();

        JMenuItem copyNameItem = new JMenuItem("Copy Name");
        copyNameItem.addActionListener(e -> {
            Toolkit.getDefaultToolkit().getSystemClipboard().setContents(
                    new StringSelection(jarDetails.getHighlightedName()), null
            );
        });

        JMenuItem copyPathItem = new JMenuItem("Copy Path");
        copyPathItem.addActionListener(e -> {
            Toolkit.getDefaultToolkit().getSystemClipboard().setContents(
                    new StringSelection(jarDetails.getPath().toString()), null
            );
        });

        JMenuItem copyMethodsItem = new JMenuItem("Copy Methods");
        copyMethodsItem.addActionListener(e -> {
            try (JarFile jarFile = new JarFile(jarDetails.getPath().toFile())) {
                List<String> methods = extractMethods(jarFile);
                Toolkit.getDefaultToolkit().getSystemClipboard().setContents(
                        new StringSelection(String.join(", ", methods)), null
                );
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        });

        JMenuItem copyAllItem = new JMenuItem("Copy All");
        copyAllItem.addActionListener(e -> {
            StringBuilder allInfo = new StringBuilder();
            allInfo.append("Name: ").append(jarDetails.getHighlightedName()).append("\n");
            allInfo.append("Path: ").append(jarDetails.getPath()).append("\n");
            allInfo.append("Last Used: ").append(new Date(jarDetails.getLastModified())).append("\n");
            allInfo.append("Size: ").append(jarDetails.getSize() / (1024.0 * 1024.0)).append(" MB\n");

            try (JarFile jarFile = new JarFile(jarDetails.getPath().toFile())) {
                List<String> methods = extractMethods(jarFile);
                allInfo.append("Methods: ").append(String.join(", ", methods));
            } catch (IOException ex) {
                ex.printStackTrace();
            }

            Toolkit.getDefaultToolkit().getSystemClipboard().setContents(
                    new StringSelection(allInfo.toString()), null
            );
        });

        contextMenu.add(copyNameItem);
        contextMenu.add(copyPathItem);
        contextMenu.add(copyMethodsItem);
        contextMenu.add(copyAllItem);

        contextMenu.show(list, point.x, point.y);
    }
}
