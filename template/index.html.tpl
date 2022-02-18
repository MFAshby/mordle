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
            <p class="headline">Mordle!</p>
            <p class="tagline"><em>M</em>artin's W<em>ordle</em> Clone</p>

            <p class="flash">{{flash}}</a></p>

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

            <p>
            <details>
            <summary>how to play</summary>
            <p>Type a 5 letter word in the box above and press enter.</p>
            <p>Correct letters will appear in <span class="correct">green</span>.</p>
            <p>Letters which are present but in the wrong position will appear in <span class="present_wrong_pos">yellow</span>.</p>
            <p>Letters which are not correct will appear in <span class="incorrect">grey</span>.</p>
            <p>You get 6 guesses.</p>
            <p>There is a new word to guess every day.</p>
            <p>See the <a href="https://www.powerlanguage.co.uk/wordle/">original wordle</a></p>
            </details>
            </p>

            {{#user}}
            {{#anon}}
            <p>
            <details>
            <summary>sign up / login to play on multiple devices and appear on the future leaderboard</summary>
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
                    <td></td>
                    <td>
                        <input type="submit" formaction="/signup" value="sign up"></input>
                        <input type="submit" formaction="/login" value="login"></input>
                    </td>
                </tr>
                </table>
            </form>
            </details>
            </p>
            {{/anon}}
            {{^anon}}
            <p>welcome {{name}}</p>
            <form method="post" action="/logout"><input type="submit" value="logout"></input></form>
            {{/anon}}
            {{/user}} 
        </div>
    </body>
</html>