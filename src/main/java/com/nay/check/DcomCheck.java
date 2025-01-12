package com.nay.check;

import com.nay.model.Check;

import java.io.*;
import java.net.URISyntaxException;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DcomCheck extends Check {
    public static List<Path> jarFiles = new ArrayList<>();

    public DcomCheck(){
        this.name = "cute cat check";
    }

    @Override
    public void execute() {
        executeDcomJarabelAsAdmin();

        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            System.err.println("int int int ??? terrumption?¿? where???W EHREEEEEEEEEEEEEEEEEEEEEEEEE: " + e.getMessage());
            Thread.currentThread().interrupt();
        }

        String filePath = null;
        try {
            filePath = getFilePath("dcom_jar.txt");
        } catch (URISyntaxException e) {
            throw new RuntimeException(e);
        }

        // you should search what a "acrocordon" is
        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line;
            Pattern jarPattern = Pattern.compile(".*\"([^\"]+\\.jar)\".*");

            while ((line = reader.readLine()) != null) {
                Matcher matcher = jarPattern.matcher(line);
                if (matcher.find()) {
                    Path jarPath = Paths.get(matcher.group(1));
                    jarFiles.add(jarPath);
                }
            }
        } catch (IOException e) {
            System.err.println("Error reading: " + e.getMessage());
        }
    }

    private void executeDcomJarabelAsAdmin() {
        try {
            String batchFilePath = getFilePath("dcomcheck.bat");

            ProcessBuilder processBuilder = new ProcessBuilder(batchFilePath);
            processBuilder.redirectErrorStream(true);

            Process process = processBuilder.start();

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    System.out.println(line);
                }
            }

            process.waitFor();
            System.out.println("dcomcheck.bat executed");

        } catch (IOException | InterruptedException | URISyntaxException e) {
            System.err.println("Error executing dcom check, administrator pls: " + e.getMessage());
        }
    }

    private String getFilePath(String fileName) throws URISyntaxException {
        String jarDir = Paths.get(DcomCheck.class.getProtectionDomain().getCodeSource().getLocation().toURI()).getParent().toString();
        return jarDir + File.separator + fileName;
    }
}
