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
    constraint name_idx unique(name)
);

create table session {
    game_user_id bigint not null reference game_user(id),
    session_token text not null,
    csrf_token text not null,
};

create table guess (
    game_user_id bigint not null references game_user(id),
    answer_date date not null references answer(answer_date),
    word text not null references wordlist(word),
    idx int not null,
    constraint user_id_answer_date_idx unique (game_user_id, answer_date, idx)
);
-- TODO check constraint restricting to just 6 turns?

drop table guess;
drop table game_user;
drop table answer;
drop table wordlist;