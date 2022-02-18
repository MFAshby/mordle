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
            <p>Mordle! (Martin's wordle clone)</p>
            {{#won}}
            <p>You have won! Well done! Play again tomorrow</p>
            {{/won}}

            {{#lost}}
            <p>You have lost! Try again tomorrow</p>
            {{/lost}}

            <table class="grid">
            {{#turns}}
            <tr>
                {{#guess}}
                <td class="letter {{state}}"><span class="letter_content">{{letter}}</span></td>
                {{/guess}}
            </tr>
            {{/turns}}
            </table>

            {{^won}}
            {{^lost}}
            <form action="/guess" method="post">
            <input class="guess_input" name="guess" autofocus required maxlength="5"></input>
            </form>
            {{/lost}}
            {{/won}}

            {{#user}}
            {{#anon}}
            <p>sign up / login to play on multiple devices and appear on the future leaderboard</p>
            <form method="post">
                <table>
                <tr>
                    <td><label for="user_name">name</label></td>
                    <td><input type="text" name="user_name"></input></td>
                </tr>
                <tr>
                    <td><label for="password">password</label></td>
                    <td><input type="password" name="password"></input></td>
                </tr>
                <tr>
                    <td colspan="2">
                        <input type="submit" formaction="/signup" value="sign up"></input>
                        <input type="submit" formaction="/login" value="login"></input>
                    </td>
                </tr>
                </table>
            </form>
            {{/anon}}
            {{^anon}}
            <p>welcome {{name}}</p>
            <form method="post" action="/logout"><input type="submit" value="logout"></input></form>
            {{/anon}}
            {{/user}} 
        </div>
    </body>
</html>