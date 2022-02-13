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
    </body>
</html>