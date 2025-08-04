package com.nay.check;

import com.nay.model.Check;

import java.io.File;
import java.io.IOException;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;

public class PrefetchCheck extends Check {

    // static static static clap clap clap
    private static final String PREFETCH_DIR = "C:\\Windows\\Prefetch";
    public static final List<Path> prefetchJars = new ArrayList<>();

    public PrefetchCheck(){
        this.name = "Prefetch Check";
    }

    @Override
    public void execute() {
        try {
            File prefetchDirectory = new File(PREFETCH_DIR);
            if (!prefetchDirectory.exists() || !prefetchDirectory.canRead()) {
                System.err.println("cant read prefetch maybe admin");
                return;
            }

            List<File> recentPrefetchFiles = Files.list(Paths.get(PREFETCH_DIR))
                    .map(Path::toFile)
                    .sorted(Comparator.comparingLong(File::lastModified).reversed())
                    .limit(150)
                    .collect(Collectors.toList());

            if (recentPrefetchFiles.isEmpty()) {
                System.out.println("? not ofund");
            } else {
                for (File pfFile : recentPrefetchFiles) {
                    if (pfFile.getName().toLowerCase().contains("java") || pfFile.getName().toLowerCase().contains("javaw")) {
                        prefetchJars.add(pfFile.toPath());
                        System.out.println(pfFile.toPath());
                    }

                }
            }

        } catch (IOException e) {
            System.err.println("error" + e.getMessage());
        }
    }
}
