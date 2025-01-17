package com.nay.model;

import lombok.Getter;

import java.nio.file.Path;

@Getter
public class JarDetails {
    private final Path path;
    private final long size;
    private final long lastModified;
    private final String highlightedName;

    public JarDetails(Path path, long size, long lastModified) {
        this(path, size, lastModified, path.getFileName().toString());
    }

    public JarDetails(Path path, long size, long lastModified, String highlightedName) {
        this.path = path;
        this.size = size;
        this.lastModified = lastModified;
        this.highlightedName = highlightedName;
    }

    @Override
    public String toString() {
        return highlightedName;
    }
}