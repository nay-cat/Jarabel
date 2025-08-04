package com.nay.model;

public abstract class Check {

    protected String name;

    public Check() {
        this.name = "";
    }

    public Check(String name) {
        this.name = name;
    }

    public abstract void execute();

    public String getName() {
        return name;
    }
}