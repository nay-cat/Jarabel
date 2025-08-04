package com.nay.check;

import com.nay.model.Check;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class RecentCheck extends Check {

    // stackoverflow stolen stolen
    public static List<String> paths = new ArrayList<>();
    private static final String REGQUERY_UTIL = "reg query ";
    private static final String RECENT_DOCS_JAR_CMD = REGQUERY_UTIL +
            "\"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs\\.jar\"";

    public RecentCheck(){
        this.name = "RecentDocs Check";
    }

    public static List<String> recentPathList = new ArrayList<>();
    // stackoverflow stolen stolen
    public static List<String> getRecentDocsJarValues() {
        try {
            Process process = Runtime.getRuntime().exec(RECENT_DOCS_JAR_CMD);
            StreamReader reader = new StreamReader(process.getInputStream());

            reader.start();
            process.waitFor();
            reader.join();

            String result = reader.getResult();
            parseRecentDocsResult(result);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return recentPathList;
    }

    private static void parseRecentDocsResult(String result) {
        String[] lines = result.split("\n");

        for (String line : lines) {
            if (line.contains("REG_BINARY")) {
                String[] parts = line.trim().split("\\s+");
                if (parts.length >= 3) {
                    StringBuilder binaryData = new StringBuilder();

                    for (int i = 2; i < parts.length; i++) {
                        binaryData.append(parts[i]);
                    }

                    String readableText = convertBinaryToText(binaryData.toString().trim());

                    if (!readableText.isEmpty()) {
                        recentPathList.add(readableText);
                    }
                }
            }
        }
    }

    // stackoverflow stolen stolen
    private static String convertBinaryToText(String hexData) {
        try {
            int length = hexData.length();
            byte[] data = new byte[length / 2];
            for (int i = 0; i < length; i += 2) {
                data[i / 2] = (byte) ((Character.digit(hexData.charAt(i), 16) << 4)
                        + Character.digit(hexData.charAt(i + 1), 16));
            }

            return new String(data, StandardCharsets.UTF_16LE).trim();
        } catch (Exception e) {
            e.printStackTrace();
            return "[Error al convertir]";
        }
    }

    @Override
    public void execute() {
        paths = getRecentDocsJarValues();
    }

    static class StreamReader extends Thread {
        private InputStream is;
        private StringWriter sw;

        StreamReader(InputStream is) {
            this.is = is;
            sw = new StringWriter();
        }

        public void run() {
            try {
                int c;
                while ((c = is.read()) != -1) {
                    sw.write(c);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        String getResult() {
            return sw.toString();
        }
    }

}
