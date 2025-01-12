package com.nay.inject.module;

import com.nay.check.DcomCheck;
import com.nay.check.PrefetchCheck;
import com.nay.check.RecentCheck;
import com.nay.check.SearchCheck;
import com.nay.model.Check;
import team.unnamed.inject.AbstractModule;

public class CheckModule extends AbstractModule {

    @Override
    protected void configure() {
        System.out.println("Loaded CheckModule");
        this.multibind(Check.class)
                .asSet()
                .to(SearchCheck.class)
                .to(PrefetchCheck.class)
                .to(DcomCheck.class)
                .to(RecentCheck.class)
                .singleton();
    }

}
