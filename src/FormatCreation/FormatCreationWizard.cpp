#include "FormatCreationWizard.h"
#include "ui_FormatCreationWizard.h"


FormatCreationWizard::FormatCreationWizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::FormatCreationWizard)
{
    ui->setupUi(this);

    connect(ui->commentsPage, &CommentWizardPage::handleError, this, &FormatCreationWizard::handleError);
    connect(ui->filenameFormatPage, &FilenameFormatWizardPage::handleError, this, &FormatCreationWizard::handleError);
    connect(ui->logFormatPage, &LogFormatWizardPage::handleError, this, &FormatCreationWizard::handleError);
    connect(ui->logFieldsPage, &LogFieldsWizardPage::handleError, this, &FormatCreationWizard::handleError);
    connect(ui->moduleListPage, &ModuleListWizardPage::handleError, this, &FormatCreationWizard::handleError);
}

FormatCreationWizard::~FormatCreationWizard()
{
    delete ui;
}

Format FormatCreationWizard::getFormat()
{
    Format format;

    format.name = field("name").toString();

    auto filenameRegex = field("logFileRegex").toString();
    filenameRegex.replace("{module}", "(?<module>[[:alnum:]]*)");
    filenameRegex.replace("{time}", "(?<time>\\d*)");
    format.logFileRegex = QRegularExpression{ field("logFileRegex").toString() };

    format.extension = field("extension").toString();
    format.encoding = QStringConverter::encodingForName(field("encoding").toString().toUtf8());

    auto modules = field("modules").toStringList();
    format.modules.insert(modules.begin(), modules.end());

    auto comments = field("comments").toList();
    for (const auto& comment : std::as_const(comments))
    {
        if (!comment.isValid() || !comment.canConvert<QVariantMap>())
            continue;

        auto commentMap = comment.toMap();
        auto& newComment = format.comments.emplace_back();
        newComment.start = commentMap.value("start").toString();
        if (commentMap.contains("finish"))
            newComment.finish = commentMap.value("finish").toString();
    }

    if (field("separatorUsage").toBool())
        format.separator = field("separator").toString();
    if (field("lineRegexUsage").toBool())
        format.lineRegex = QRegularExpression{ field("lineRegex").toString() };

    format.timeFieldIndex = field("timeFieldIndex").toInt();
    format.timeMask = field("timeMask").toString();
    format.timeFractionalDigits = field("timeFractionalDigits").toInt();

    auto fields = field("fields").toList();
    for (const auto& field : std::as_const(fields))
    {
        if (field.metaType().id() == QMetaType::Type::QVariantMap)
        {
            auto fieldMap = field.toMap();
            format.fields.emplace_back(
                fieldMap.value("name").toString(),
                QRegularExpression{ fieldMap.value("regex").toString() },
                static_cast<QMetaType::Type>(fieldMap.value("type").toInt())
            );
        }
    }

    return format;
}

int FormatCreationWizard::nextId() const
{
    if (currentPage() == ui->filenameFormatPage)
    {
        auto logFileRegex = field("logFileRegex").toString();
        if (logFileRegex.isEmpty() || logFileRegex.contains("module"))
            return 1;
        else
            return 2;
    }

    return QWizard::nextId();
}
