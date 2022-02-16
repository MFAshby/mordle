<!doctype HTML>
<html>
    <head>
        <meta charset="UTF-8">
        <!-- helps with mobile view -->
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <link rel="stylesheet" type="text/css" href="main.css"/>
    </head>
    <body>
        <div class="container">
            <!-- TODO i18n -->
            <p>Mordle! (Martin's wordle clone)</p>
            {{#won}}
            <p>You have won! Well done! Play again tomorrow</p>
            {{/won}}

            {{#lost}}
            <p>You have lost! Try again tomorrow</p>
            {{/lost}}
            <div class="grid">
            {{#turns}}
                {{#guess}}
                <div class="letter {{state}}"><div class="letter_content">{{letter}}</div></div>
                {{/guess}}
            {{/turns}}
            </div>

            {{^won}}
            {{^lost}}
            <form action="/guess" method="post">
            <input class="guess_input" name="guess" autofocus required maxlength="5"></input>
            </form>
            {{/lost}}
            {{/won}}
        <div>
        <div class="signup">
            <p>welcome {{#user}}{{name}}{{/user}}</p>
            {{#anon}}
            <p>want to play on another device? enter a name and password here and click 'sign up'</p>
            <form>
                <input type="text" name="user_name"></input>
                <input type="password" name="password"></input>
                <input type="submit" formaction="/signup" value="sign up"></input>
            </form>

            <p>if you already have a name and password, enter it and click 'Login' to continue your game here</p>
            <form>
                <input type="text" name="user_name"></input>
                <input type="password" name="password"></input>
                <input type="submit" formaction="/login" value="login"></input>
            </form>
            {{/anon}}
            {{^anon}}
            <form><input type="submit" formaction="/logout" value="logout"></input></form>
            {{/anon}}

            {{#leaderboard}}
            <p>TODO display leaderboard here</p>
            <p>want to get off the leaderboard?</p>
            <form><input type="submit" formaction="/leaveleaderboard" value="logout"></input></form>
            {{/leaderboard}}
            {{^leaderboard}}
            <p>want to be on the leaderboard?</p>
            <form><input type="submit" formaction="/joinleaderboard" value="logout"></input></form>
            {{/leaderboard}}
        </div>
    </body>
</html>