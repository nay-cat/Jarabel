package com.nay.check;

import com.nay.Jarabel;
import com.nay.model.Check;

import java.io.File;
import java.net.URISyntaxException;
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.ForkJoinPool;

import static com.nay.Jarabel.searchAllDrives;
import static com.nay.utils.CheckUtils.analyzeJarsInParallel;
import static com.nay.utils.CheckUtils.collectJarFiles;

public class SearchCheck extends Check {

    // static static static clap clap clap
    public static final Set<Path> jarFiles = Collections.synchronizedSet(new HashSet<>());
    public static final ForkJoinPool pool = new ForkJoinPool(Runtime.getRuntime().availableProcessors());

    public SearchCheck(){
        this.name = "Search Check";
    }

    @Override
    public void execute()  {
        Path rootDirectory = Paths.get("C:\\");

        if (Jarabel.here){
            try {
                rootDirectory = Paths.get(new File(SearchCheck.class.getProtectionDomain().getCodeSource().getLocation().toURI()).getParent());
            } catch (URISyntaxException e) {
                throw new RuntimeException(e);
            }
        }

        if (searchAllDrives) {
            System.out.println("Searching all drives...");
            FileSystem fileSystem = FileSystems.getDefault();
            for (Path root : fileSystem.getRootDirectories()) {
                System.out.println("Scanning drive: " + root.toString());
                collectJarFiles(root);
            }
        } else {
            collectJarFiles(rootDirectory);
        }

        analyzeJarsInParallel();
    }
}
