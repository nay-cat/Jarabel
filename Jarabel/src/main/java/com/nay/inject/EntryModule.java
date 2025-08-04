package com.nay.inject;

import com.nay.inject.module.CheckModule;
import com.nay.inject.module.ServiceModule;
import team.unnamed.inject.AbstractModule;

public class EntryModule extends AbstractModule {

    @Override
    public void configure(){
        this.install();
        this.install(new CheckModule());
        this.install(new ServiceModule());
    }
}
