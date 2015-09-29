#include <memory>
#include <list>
#include <string>
#include <iostream>

namespace sexp {
    struct pos {
        unsigned line, col;
    };

    typedef std::list<struct Elem *> elem_list;

    struct Elem {
        struct pos start, end;

        virtual const elem_list& children() {
            static const elem_list empty;

            return empty;
        }
    };

    struct Atom : Elem {
        std::string str;
    };

    struct List : Elem {
        elem_list list;
        virtual const elem_list& children() { return list; }
        List* append(Elem* e) { list.push_back(e); return this; }
    };

    static const std::string ws = " \t\r\n";
    static const std::string excluded = ws + std::string("();") + '\0';

    struct parser {
        std::istream& input;
        struct pos pos;
        const char *expecting = NULL;

        parser(std::istream& input)
            : input(input) {
            pos.line = 1;
            pos.col = 1;
        }

        int
        get() {
            int ch = this->input.get();

            if (ch == '\n') {
                this->pos.line++;
                this->pos.col = 1;
            } else {
                this->pos.col++;
            }

            return (ch);
        }

        int
        peek() {
            return (this->input.peek());
        }

        bool
        good() const {
            return (this->input.good());
        }

        void
        skip_ws() {
            while (ws.find(peek()) != std::string::npos && good())
                get();
        }

        Elem*
        parse_sexp() {
            skip_ws();
            Elem* e = NULL;
            if (peek() == '"')
                e = parse_string();
            else if (peek() == '(')
                e = parse_list();
            else
                e = parse_atom();
            return (e);
        }

        Elem*
        parse_list() {
            std::auto_ptr<List> l(new List());
            l->start = pos;

            get();
            for (;;) {
                skip_ws();
                if (peek() == ')')
                    break;
                if (!good()) {
                    expecting = ")";
                    return (NULL);
                }
                Elem* e = parse_sexp();
                if (e)
                    l->append(e);
            }
            get();
            l->end = pos;

            return (l.release());
        }

        Elem*
        parse_string() {
            std::auto_ptr<Atom> a(new Atom());
            a->start = pos;
            get();

            expecting = "\"";

            while (peek() != '"') {
                int ch = get();
                switch (ch) {
                case '\\':
                    ch = get();
                    break;
                }

                if (!good())
                    return (NULL);
                a->str += ch;
            }
            get();
            a->end = pos;

            return (a.release());
        }

        Elem*
        parse_atom() {
            std::auto_ptr<Atom> a(new Atom());
            a->start = pos;

            while (excluded.find(peek()) == std::string::npos && good())
                a->str += get();
            a->end = pos;

            if (a->str.empty())
                return (NULL);
            return (a.release());
        }
    };
}

void
print_recurse(sexp::Elem* sexp)
{
    sexp::Atom* a = dynamic_cast<sexp::Atom*>(sexp);
    sexp::List* l = dynamic_cast<sexp::List*>(sexp);

    if (a) {
        std::cout << a->str;
    } else if (l) {
        bool first = true;

        std::cout << '(';
        for (sexp::elem_list::const_iterator i = l->children().begin();
             i != l->children().end();
             ++i) {
            if (!first)
                std::cout << ' ';
            print_recurse(*i);
            first = false;
        }
        std::cout << ')';
    }
}

int
main(void)
{
    sexp::parser p(std::cin);
    sexp::Elem* sexp = p.parse_sexp();

    if (sexp) {
        print_recurse(sexp);
    } else {
        std::cerr << "parse error at " << p.pos.line << ":" << p.pos.col << ": " << p.expecting << std::endl;
    }

    return (0);
}
