create table wordlist (
    word text not null primary key
);

create table answer (
    answer_date date not null primary key,
    word text references wordlist(word)
);

create table game_user (
    id bigserial not null primary key,
    name text not null,
    password_hash text,
    anon bool not null default true,
    constraint name_idx unique(name)
);

create table session (
    game_user_id bigint not null references game_user(id),
    session_token text not null,
    constraint session_session_token_idx unique(session_token)
);

create table guess (
    game_user_id bigint not null references game_user(id),
    answer_date date not null references answer(answer_date),
    word text not null references wordlist(word),
    idx int not null,
    constraint user_id_answer_date_idx unique (game_user_id, answer_date, idx)
);
-- TODO check constraint restricting to just 6 turns?

create table game_result (
    game_user_id bigint not null references game_user(id),
    answer_date date not null references answer(answer_date),
    win bool not null,
    score int not null
);

-- the above is denormalized data, here's a script to re-populate it
truncate table game_result;
with gg as (
    select g.game_user_id, 
        g.answer_date, 
        g.word = a.word as win 
    from guess g 
    join answer a on a.answer_date = g.answer_date
)
insert into game_result
select game_user_id, 
        answer_date, 
        max(win::int)::bool as win, 
        count(*) as score 
from gg group by game_user_id, answer_date;

--#drop table guess;
--#drop table game_user;
--#drop table answer;
--#drop table wordlist;


-- leaderboard query, create a deliberate cartesian of game_user and the date