/* Copyright (c) 2021, sarutora
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "highlighter.h"

QVector<BracketInfo *> TextBlockData::brackets()
{
    return m_brackets;
}

QVector<QPair<char, char>> TextBlockData::pairs()
{
    return {{'(', ')'}, {'[', ']'}, {'{', '}'}};
}


void TextBlockData::insert(BracketInfo *info)
{
    int i = 0;
    while (i < m_brackets.size() &&
        info->position > m_brackets.at(i)->position)
        ++i;

    m_brackets.insert(i, info);
}

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(QColor(228, 170, 12));
    QStringList keywordPatterns;
    keywordPatterns << "\\bas\\b" << "\\basync\\b" << "\\bawait\\b"
                    << "\\bbreak\\b" << "\\bconst\\b" << "\\bcontinue\\b"
                    << "\\bcrate\\b" << "\\bdyn\\b" << "\\belse\\b"
                    << "\\benum\\b" << "\\bextern\\b" << "\\bfn\\b"
                    << "\\bfor\\b" << "\\bif\\b" << "\\bimpl\\b"
                    << "\\bin\\b" << "\\blet\\b" << "\\bloop\\b"
                    << "\\bmatch\\b" << "\\bmod\\b" << "\\bpub\\b"
                    << "\\bref\\b" << "\\breturn\\b" << "\\bstatic\\b"
                    << "\\bstruct\\b" << "\\bsuper\\b" << "\\btrait\\b"
                    << "\\btype\\b" << "\\bunion\\b" << "\\bunsafe\\b"
                    << "\\buse\\b" << "\\bwhere\\b" <<"\\bwhile\\b"
                    << "\\|" << "\\=" << "\\+"
                    << "\\-" << "\\<" << "\\>"
                    << "\\!" << "\\&" << "\\&\\&";
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    keywordFormat.setForeground(QColor(134, 253, 174));
    QStringList keywordPatterns2;
    keywordPatterns2 << "\\bbool\\b" << "\\bchar\\b" << "\\bf32\\b"
                     << "\\bf64\\b" << "\\bi8\\b" << "\\bi16\\b"
                     << "\\bi32\\b" << "\\bi64\\b" << "\\bi128\\b"
                     << "\\bisize\\b" << "\\bstr\\b" << "\\bu8\\b"
                     << "\\bu16\\b" << "\\bu32\\b" << "\\bu64\\b"
                     << "\\bu128\\b" << "\\busize\\b" << "\\bmut\\b"
                     << "\\bmove\\b" << "\\*" << "\\&(?=[A-Za-z0-9_'])"
                     << "\\bSelf\\b" << "\\bCopy\\b" << "\\bClone\\b"
                     << "\\bDebug\\b" << "\\bString\\b" << "\\bVec\\b";
    foreach (const QString &pattern, keywordPatterns2) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    literalFormat.setForeground(QColor(192,97,203));
    rule.pattern = QRegularExpression("\\b[0-9]*");
    rule.format = literalFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]\\.[0-9_]*");
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\bself\\b");
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]_[0-9_]*");
    highlightingRules.append(rule);
    QStringList keywordPatterns3;
    keywordPatterns3 << "f32\\b" << "f64\\b" << "i8\\b"
                     << "i16\\b" << "i32\\b" << "i64\\b"
                     << "i128\\b" << "isize\\b" << "u8\\b"
                     << "u16\\b" << "u32\\b" << "u64\\b"
                     << "u128\\b" << "usize\\b";
    foreach (const QString &pattern, keywordPatterns3) {
        rule.pattern = QRegularExpression("\\b[0-9]*_" + pattern);
        rule.format = literalFormat;
        highlightingRules.append(rule);
    }

    functionFormat.setForeground(QColor(94, 212, 251));
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+\\!");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=::.)");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    functionFormat.setFontWeight(QFont::Bold);
    functionFormat.setForeground(QColor(51, 199, 222));
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    QStringList keywordPatterns4;
    keywordPatterns4 << "(?<=struct)" << "(?<=trait)" << "(?<=enum)"
                     << "(?<=fn)";
    foreach (const QString &pattern, keywordPatterns4) {
        rule.pattern = QRegularExpression(pattern + "\\s*[A-Za-z0-9_]+");
        rule.format = functionFormat;
        highlightingRules.append(rule);
    }

    literalFormat.setForeground(QColor(234,198,198));
    rule.pattern = QRegularExpression("::");
    rule.format = literalFormat;
    highlightingRules.append(rule);

    literalFormat.setForeground(QColor(192,97,203));
    rule.pattern = QRegularExpression("'\\\\?.'");
    rule.format = literalFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(QColor(51, 199, 222));
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#\\[[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    escapeFormat.setForeground(QColor(234,198,198));
    rule.pattern = QRegularExpression("(?<!/)///(?!/)[^\n]*");
    rule.format = escapeFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//\\![^\n]*");
    rule.format = escapeFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(QColor(192,97,203));
    quoteStartExpression = QRegularExpression("(?<!')\"(?!')");
    quoteEndExpression = QRegularExpression("\"");
    escapeExpression = QRegularExpression("\\\\");
    controlExpression = QRegularExpression("\\\\.");
    lifetimeExpression = QRegularExpression("'[A-Za-z0-9_]+(?!')");

}

QVector<QRegularExpressionMatch> Highlighter::findEscape(const QString &text) {
    int startIndex = 0;
    QVector<QRegularExpressionMatch> matches;
    while (startIndex != -1) {
        QRegularExpressionMatch match = escapeExpression.match(text, startIndex);
        if (match.capturedStart() != -1) {
            matches.push_back(match);
            startIndex = match.capturedEnd()+1;
        } else {
            break;
        }
    }
    return matches;
}

void Highlighter::highlightBlock(const QString &text)
{
    TextBlockData *data = new TextBlockData;

    int commentPos = text.indexOf("//");
    for (const auto pair: data->pairs()) {
        int leftPos = text.indexOf(pair.first);
        while (leftPos != -1) {
            if (commentPos == -1 || leftPos < commentPos) {
                BracketInfo *info = new BracketInfo;
                info->character = pair.first;
                info->position = leftPos;

                data->insert(info);
            }
            leftPos = text.indexOf(pair.first, leftPos + 1);
        }
        int rightPos = text.indexOf(pair.second);
        while (rightPos != -1) {
            if (commentPos == -1 || rightPos < commentPos) {
                BracketInfo *info = new BracketInfo;
                info->character = pair.second;
                info->position = rightPos;

                data->insert(info);
            }

            rightPos = text.indexOf(pair.second, rightPos +1);
        }
    }
    setCurrentBlockUserData(data);

    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    auto insertChar = [&](char c, int position) {
        BracketInfo *info = new BracketInfo;
        info->character = c;
        info->position = position;
        data->insert(info);
    };

    setCurrentBlockState(0);

    int commentIndex = text.indexOf("//");
    int startIndex = 0;
    int prev = previousBlockState();
    auto findQuote = [&](int n) {
        if (commentIndex == -1) {
            startIndex = text.indexOf(quoteStartExpression, n);
            if (startIndex != -1) {
                insertChar('\'', startIndex);
            }
        } else {
            int quoteIndex = text.indexOf(quoteStartExpression, n);
            if (quoteIndex > 0 && quoteIndex < commentIndex) {
                startIndex = quoteIndex;
                insertChar('\'', startIndex);
            } else
                startIndex = -1;
        }
    };
    if (prev != 1) {
        findQuote(startIndex);
    }

    auto findChar = [&](int n) {
            while (n != -1) {
                QRegularExpressionMatch match = controlExpression.match(text, n);
                n = match.capturedStart();
                if (n != -1 && (commentIndex == -1 || n < commentIndex)) {
                    QChar c = text.at(n+1);
                    if (c == 'n' || c == 'r' || c == 't')
                        setFormat(n, 2, escapeFormat);
                    else
                        setFormat(n, 2, basicFormat);
                    n = match.capturedEnd();
                }
                else
                    break;
            }
    };
    auto findLifetime = [&](int n) {
            while (n != -1) {
                n = text.indexOf('\'', n);
                if (n != -1 && (commentIndex == -1 || n < commentIndex)) {
                    QRegularExpressionMatch match = lifetimeExpression.match(text, n);
                    int m = match.capturedEnd();
                    if (m != -1) {
                        setFormat(n, match.capturedLength(), escapeFormat);
                        n = m;
                    }
                    else
                        break;
                }
                else
                    break;
            }
    };

    findChar(0);
    findLifetime(0);
    QVector<QRegularExpressionMatch> escapes = findEscape(text);

    bool b = false;
    while (startIndex >= 0) {
        QRegularExpressionMatch match;
        if (prev != 1 || b) {
            match = quoteEndExpression.match(text, startIndex+1);
        } else {
            match = quoteEndExpression.match(text, startIndex);
        }
        int endIndex = match.capturedStart();

        int n = 0;
        int quoteLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            quoteLength = text.length() - startIndex;
            n = escapes.length();
        } else {
            bool b = false;
            for (const auto &escape: escapes) {
                int escEnd = escape.capturedEnd();
                if (escEnd < endIndex) {
                    n++;
                } else if (escEnd == endIndex) {
                    setFormat(startIndex, escape.capturedStart()-startIndex, quotationFormat);
                    for (int i = 0; i < n; i++) {
                        const auto& e = escapes.at(i);
                        setFormat(e.capturedStart(), 2, escapeFormat);
                    }
                    startIndex = endIndex+1;
                    escapes.remove(0, n);
                    b = true;
                    break;
                } else {
                    break;
                }
            }
            if (b)
                continue;

            insertChar('"', endIndex);
            quoteLength = endIndex - startIndex
                            + match.capturedLength();
        }
        b = true;
        setFormat(startIndex, quoteLength, quotationFormat);
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                const auto escape = escapes.at(i);
                setFormat(escape.capturedStart(), escape.capturedLength()+1, escapeFormat);
            }
            if (n < escapes.length())
                escapes.remove(0, n);
        }
        int endQuote = startIndex+quoteLength;
        findQuote(startIndex+quoteLength);
        findChar(endQuote);
        findLifetime(endQuote);
    }
}
