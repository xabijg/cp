-module(break_md5).
-define(PASS_LEN, 6).
-define(UPDATE_BAR_GAP, 1000).
-define(BAR_SIZE, 40).

-define(NUM_THREADS,8).          %creo los threads

-export([break_md5s/1, pass_to_num/1, num_to_pass/1]).
-export([progress_loop/2]).
-export([break_md5/5]).

% Base ^ Exp

pow_aux(_Base, Pow, 0) -> Pow;
pow_aux(Base, Pow, Exp) when Exp rem 2 == 0 ->
    pow_aux(Base*Base, Pow, Exp div 2);
pow_aux(Base, Pow, Exp) -> pow_aux(Base, Base * Pow, Exp - 1).

pow(Base, Exp) -> pow_aux(Base, 1, Exp).

%% Number to password and back conversion

num_to_pass_aux(_N, 0, Pass) -> Pass;
num_to_pass_aux(N, Digit, Pass) ->
    num_to_pass_aux(N div 26, Digit - 1, [$a + N rem 26 | Pass]).

num_to_pass(N) -> num_to_pass_aux(N, ?PASS_LEN, []).

pass_to_num_aux([], Num) -> Num;
pass_to_num_aux([C|T], Num) -> pass_to_num_aux(T, Num*26 + C-$a).

pass_to_num(Pass) -> pass_to_num_aux(Pass, 0).

%% Hex string to Number

hex_char_to_int(N) ->
    if (N >= $0) and (N =< $9) -> N-$0;
       (N >= $a) and (N =< $f) -> N-$a+10;
       (N >= $A) and (N =< $F) -> N-$A+10;
       true -> throw({not_hex, [N]})
    end.

hex_string_to_num_aux([], Num) -> Num;
hex_string_to_num_aux([Hex|T], Num) ->
    hex_string_to_num_aux(T, Num*16 + hex_char_to_int(Hex)).

hex_string_to_num(Hex) -> hex_string_to_num_aux(Hex, 0).

%% Progress bar runs in its own process

progress_loop(N, Bound) ->
    receive
        stop -> ok;
        {progress_report, Checked} ->
            N2 = N + Checked,
            Full_N = N2 * ?BAR_SIZE div Bound,
            Full = lists:duplicate(Full_N, $=),
            Empty = lists:duplicate(?BAR_SIZE - Full_N, $-),
            io:format("\r[~s~s] ~.2f%", [Full, Empty, N2/Bound*100]),
            progress_loop(N2, Bound)
    end.

%% break_md5/5 iterates checking the possible passwords

break_md5([], Current, End, Progress_Pid, []) ->
    receive
        {add, Pids} -> break_md5([], Current, End, Progress_Pid, Pids)
    end;
break_md5([], _, _, _,Pid_List) -> Pid_List!{stop,self()}; % No more hashes to find
break_md5(Hashes, N, N, _,Pid_List) -> Pid_List!{not_found, Hashes};  % Checked every possible password
break_md5(Hashes, Current, End, Progress_Pid, Pid_List) ->

    receive
        stop->HashesList=[],ok;
        {found,Num}->HashesList=lists:delete(Num,Hashes)               %Para y espera por los hashes
    after 0 ->HashesList=Hashes
    end,

    if Current rem ?UPDATE_BAR_GAP == 0 ->
            Progress_Pid ! {progress_report, ?UPDATE_BAR_GAP};
        true ->
            ok
    end,

    Pass = num_to_pass(Current),
    Hash = crypto:hash(md5, Pass),
    Num_Hash = binary:decode_unsigned(Hash),
    case lists:member(Num_Hash, Hashes) of
        true ->
            io:format("\e[2K\r~.16B: ~s~n", [Num_Hash, Pass]),
            Message = {found, Num_Hash},
            [Pid ! Message || Pid <- Pid_List],
            break_md5(lists:delete(Num_Hash, Hashes), Current+1, End, Progress_Pid, Pid_List);
        false ->
            break_md5(Hashes, Current+1, End, Progress_Pid, Pid_List)
    end.

%% Break a list of hashes

break_md5s(Hashes) ->
    Bound = pow(26, ?PASS_LEN),
    Progress_Pid = spawn(?MODULE, progress_loop, [0, Bound]),
    Num_Hashes = lists:map(fun hex_string_to_num/1, Hashes),
    Res = break_md5(Num_Hashes, 0, Bound, Progress_Pid, []),      %aqui cambiar por Current threads despues
    Progress_Pid ! stop,
    Res.

%% Break a single hash

break_md5(Hash) -> break_md5s([Hash]).
