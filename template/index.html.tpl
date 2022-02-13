<!doctype HTML>
<html>
    <body>
        <p>Mordle! (Martin's wordle clone)</p>
        {{#turns}}
        <p>
            {{#guess}}
            <span class="{{state}}">{{letter}}</span>
            {{/guess}}
        </p>
        {{/turns}}
        <form action="/guess" method="post">
        <input name="guess" required></input>
        </form>
    </body>
</html>