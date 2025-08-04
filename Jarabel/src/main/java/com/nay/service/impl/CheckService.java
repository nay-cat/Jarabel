package com.nay.service.impl;

import com.nay.model.Check;
import com.nay.service.Service;
import team.unnamed.inject.Inject;

import java.util.Set;

public class CheckService implements Service {

    @Inject
    private Set<Check> checks;

    @Override
    public void start() {
        checks.forEach(check -> {
            System.out.println("Executing: "+check.getName());
            check.execute();
        });
    }
}
