sou uma lib do namespace bsl
- quando declaro, declaro no bsl
- quando leio, leio do bsl e dos namescpaces adicionados a mim como lib
quando tenho adicionado um arquivo de determinado namespace, tenho acesso a tudo que foi adicionado naquele namespace?

fazer o seguinte teste:

             lib::n1
                |
              /  \
            /      \
          main    lib2::n1

main tem acesso ao fonte do lib2?
