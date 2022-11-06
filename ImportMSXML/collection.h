#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Data.dll>
#using <System.Xml.dll>

using namespace System::Security::Permissions;
[assembly:SecurityPermissionAttribute(SecurityAction::RequestMinimum, SkipVerification=false)];
// 
// 此源代码由 xsd 自动生成, Version=2.0.50727.3038。
// 
namespace ImportMSXML {
    using namespace System;
    ref class Collection;
    
    
    /// <summary>
///Represents a strongly typed in-memory cache of data.
///</summary>
    [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"2.0.0.0"), 
    System::Serializable, 
    System::ComponentModel::DesignerCategoryAttribute(L"code"), 
    System::ComponentModel::ToolboxItem(true), 
    System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedDataSetSchema"), 
    System::Xml::Serialization::XmlRootAttribute(L"Collection"), 
    System::ComponentModel::Design::HelpKeywordAttribute(L"vs.data.DataSet")]
    public ref class Collection : public ::System::Data::DataSet {
        public : ref class OtherBookDataTable;
        public : ref class OtherBookRow;
        public : ref class OtherBookRowChangeEvent;
        
        private: ImportMSXML::Collection::OtherBookDataTable^  tableOtherBook;
        
        private: ::System::Data::SchemaSerializationMode _schemaSerializationMode;
        
        public : delegate System::Void OtherBookRowChangeEventHandler(::System::Object^  sender, ImportMSXML::Collection::OtherBookRowChangeEvent^  e);
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        Collection();
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        Collection(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property ImportMSXML::Collection::OtherBookDataTable^  OtherBook {
            ImportMSXML::Collection::OtherBookDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::ComponentModel::BrowsableAttribute(true), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Visible)]
        virtual property ::System::Data::SchemaSerializationMode SchemaSerializationMode {
            ::System::Data::SchemaSerializationMode get() override;
            System::Void set(::System::Data::SchemaSerializationMode value) override;
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Hidden)]
        property ::System::Data::DataTableCollection^  Tables {
            ::System::Data::DataTableCollection^  get() new;
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Hidden)]
        property ::System::Data::DataRelationCollection^  Relations {
            ::System::Data::DataRelationCollection^  get() new;
        }
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        virtual ::System::Void InitializeDerivedDataSet() override;
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        virtual ::System::Data::DataSet^  Clone() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        virtual ::System::Boolean ShouldSerializeTables() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        virtual ::System::Boolean ShouldSerializeRelations() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        virtual ::System::Void ReadXmlSerializable(::System::Xml::XmlReader^  reader) override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        virtual ::System::Xml::Schema::XmlSchema^  GetSchemaSerializable() override;
        
        internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        ::System::Void InitVars();
        
        internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        ::System::Void InitVars(::System::Boolean initTable);
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        ::System::Void InitClass();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        ::System::Boolean ShouldSerializeOtherBook();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        ::System::Void SchemaChanged(::System::Object^  sender, ::System::ComponentModel::CollectionChangeEventArgs^  e);
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedDataSetSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"2.0.0.0"), 
        System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class OtherBookDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnTitle;
            
            private: ::System::Data::DataColumn^  columnAuthor;
            
            private: ::System::Data::DataColumn^  columnPublisher;
            
            public: event ImportMSXML::Collection::OtherBookRowChangeEventHandler^  OtherBookRowChanging;
            
            public: event ImportMSXML::Collection::OtherBookRowChangeEventHandler^  OtherBookRowChanged;
            
            public: event ImportMSXML::Collection::OtherBookRowChangeEventHandler^  OtherBookRowDeleting;
            
            public: event ImportMSXML::Collection::OtherBookRowChangeEventHandler^  OtherBookRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            OtherBookDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            OtherBookDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            OtherBookDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property ::System::Data::DataColumn^  TitleColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property ::System::Data::DataColumn^  AuthorColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property ::System::Data::DataColumn^  PublisherColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property ImportMSXML::Collection::OtherBookRow^  default [::System::Int32 ] {
                ImportMSXML::Collection::OtherBookRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            ::System::Void AddOtherBookRow(ImportMSXML::Collection::OtherBookRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            ImportMSXML::Collection::OtherBookRow^  AddOtherBookRow(System::String^  Title, System::String^  Author, System::String^  Publisher);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            ImportMSXML::Collection::OtherBookRow^  NewOtherBookRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            ::System::Void RemoveOtherBookRow(ImportMSXML::Collection::OtherBookRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"2.0.0.0")]
        ref class OtherBookRow : public ::System::Data::DataRow {
            
            private: ImportMSXML::Collection::OtherBookDataTable^  tableOtherBook;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            OtherBookRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property System::String^  Title {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property System::String^  Author {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property System::String^  Publisher {
                System::String^  get();
                System::Void set(System::String^  value);
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"2.0.0.0")]
        ref class OtherBookRowChangeEvent : public ::System::EventArgs {
            
            private: ImportMSXML::Collection::OtherBookRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            OtherBookRowChangeEvent(ImportMSXML::Collection::OtherBookRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property ImportMSXML::Collection::OtherBookRow^  Row {
                ImportMSXML::Collection::OtherBookRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
    };
}
namespace ImportMSXML {
    
    
    inline Collection::Collection() {
        this->BeginInit();
        this->InitClass();
        ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &ImportMSXML::Collection::SchemaChanged);
        __super::Tables->CollectionChanged += schemaChangedHandler;
        __super::Relations->CollectionChanged += schemaChangedHandler;
        this->EndInit();
    }
    
    inline Collection::Collection(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataSet(info, context, false) {
        if (this->IsBinarySerialized(info, context) == true) {
            this->InitVars(false);
            ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler1 = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &ImportMSXML::Collection::SchemaChanged);
            this->Tables->CollectionChanged += schemaChangedHandler1;
            this->Relations->CollectionChanged += schemaChangedHandler1;
            return;
        }
        ::System::String^  strSchema = (cli::safe_cast<::System::String^  >(info->GetValue(L"XmlSchema", ::System::String::typeid)));
        if (this->DetermineSchemaSerializationMode(info, context) == ::System::Data::SchemaSerializationMode::IncludeSchema) {
            ::System::Data::DataSet^  ds = (gcnew ::System::Data::DataSet());
            ds->ReadXmlSchema((gcnew ::System::Xml::XmlTextReader((gcnew ::System::IO::StringReader(strSchema)))));
            if (ds->Tables[L"OtherBook"] != nullptr) {
                __super::Tables->Add((gcnew ImportMSXML::Collection::OtherBookDataTable(ds->Tables[L"OtherBook"])));
            }
            this->DataSetName = ds->DataSetName;
            this->Prefix = ds->Prefix;
            this->Namespace = ds->Namespace;
            this->Locale = ds->Locale;
            this->CaseSensitive = ds->CaseSensitive;
            this->EnforceConstraints = ds->EnforceConstraints;
            this->Merge(ds, false, ::System::Data::MissingSchemaAction::Add);
            this->InitVars();
        }
        else {
            this->ReadXmlSchema((gcnew ::System::Xml::XmlTextReader((gcnew ::System::IO::StringReader(strSchema)))));
        }
        this->GetSerializationData(info, context);
        ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &ImportMSXML::Collection::SchemaChanged);
        __super::Tables->CollectionChanged += schemaChangedHandler;
        this->Relations->CollectionChanged += schemaChangedHandler;
    }
    
    inline ImportMSXML::Collection::OtherBookDataTable^  Collection::OtherBook::get() {
        return this->tableOtherBook;
    }
    
    inline ::System::Data::SchemaSerializationMode Collection::SchemaSerializationMode::get() {
        return this->_schemaSerializationMode;
    }
    inline System::Void Collection::SchemaSerializationMode::set(::System::Data::SchemaSerializationMode value) {
        this->_schemaSerializationMode = __identifier(value);
    }
    
    inline ::System::Data::DataTableCollection^  Collection::Tables::get() {
        return __super::Tables;
    }
    
    inline ::System::Data::DataRelationCollection^  Collection::Relations::get() {
        return __super::Relations;
    }
    
    inline ::System::Void Collection::InitializeDerivedDataSet() {
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline ::System::Data::DataSet^  Collection::Clone() {
        ImportMSXML::Collection^  cln = (cli::safe_cast<ImportMSXML::Collection^  >(__super::Clone()));
        cln->InitVars();
        cln->SchemaSerializationMode = this->SchemaSerializationMode;
        return cln;
    }
    
    inline ::System::Boolean Collection::ShouldSerializeTables() {
        return false;
    }
    
    inline ::System::Boolean Collection::ShouldSerializeRelations() {
        return false;
    }
    
    inline ::System::Void Collection::ReadXmlSerializable(::System::Xml::XmlReader^  reader) {
        if (this->DetermineSchemaSerializationMode(reader) == ::System::Data::SchemaSerializationMode::IncludeSchema) {
            this->Reset();
            ::System::Data::DataSet^  ds = (gcnew ::System::Data::DataSet());
            ds->ReadXml(reader);
            if (ds->Tables[L"OtherBook"] != nullptr) {
                __super::Tables->Add((gcnew ImportMSXML::Collection::OtherBookDataTable(ds->Tables[L"OtherBook"])));
            }
            this->DataSetName = ds->DataSetName;
            this->Prefix = ds->Prefix;
            this->Namespace = ds->Namespace;
            this->Locale = ds->Locale;
            this->CaseSensitive = ds->CaseSensitive;
            this->EnforceConstraints = ds->EnforceConstraints;
            this->Merge(ds, false, ::System::Data::MissingSchemaAction::Add);
            this->InitVars();
        }
        else {
            this->ReadXml(reader);
            this->InitVars();
        }
    }
    
    inline ::System::Xml::Schema::XmlSchema^  Collection::GetSchemaSerializable() {
        ::System::IO::MemoryStream^  stream = (gcnew ::System::IO::MemoryStream());
        this->WriteXmlSchema((gcnew ::System::Xml::XmlTextWriter(stream, nullptr)));
        stream->Position = 0;
        return ::System::Xml::Schema::XmlSchema::Read((gcnew ::System::Xml::XmlTextReader(stream)), nullptr);
    }
    
    inline ::System::Void Collection::InitVars() {
        this->InitVars(true);
    }
    
    inline ::System::Void Collection::InitVars(::System::Boolean initTable) {
        this->tableOtherBook = (cli::safe_cast<ImportMSXML::Collection::OtherBookDataTable^  >(__super::Tables[L"OtherBook"]));
        if (initTable == true) {
            if (this->tableOtherBook != nullptr) {
                this->tableOtherBook->InitVars();
            }
        }
    }
    
    inline ::System::Void Collection::InitClass() {
        this->DataSetName = L"Collection";
        this->Prefix = L"";
        this->EnforceConstraints = true;
        this->SchemaSerializationMode = ::System::Data::SchemaSerializationMode::IncludeSchema;
        this->tableOtherBook = (gcnew ImportMSXML::Collection::OtherBookDataTable());
        __super::Tables->Add(this->tableOtherBook);
    }
    
    inline ::System::Boolean Collection::ShouldSerializeOtherBook() {
        return false;
    }
    
    inline ::System::Void Collection::SchemaChanged(::System::Object^  sender, ::System::ComponentModel::CollectionChangeEventArgs^  e) {
        if (e->Action == ::System::ComponentModel::CollectionChangeAction::Remove) {
            this->InitVars();
        }
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  Collection::GetTypedDataSetSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ImportMSXML::Collection^  ds = (gcnew ImportMSXML::Collection());
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        ::System::Xml::Schema::XmlSchemaAny^  any = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any->Namespace = ds->Namespace;
        sequence->Items->Add(any);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline Collection::OtherBookDataTable::OtherBookDataTable() {
        this->TableName = L"OtherBook";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline Collection::OtherBookDataTable::OtherBookDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline Collection::OtherBookDataTable::OtherBookDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  Collection::OtherBookDataTable::TitleColumn::get() {
        return this->columnTitle;
    }
    
    inline ::System::Data::DataColumn^  Collection::OtherBookDataTable::AuthorColumn::get() {
        return this->columnAuthor;
    }
    
    inline ::System::Data::DataColumn^  Collection::OtherBookDataTable::PublisherColumn::get() {
        return this->columnPublisher;
    }
    
    inline ::System::Int32 Collection::OtherBookDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline ImportMSXML::Collection::OtherBookRow^  Collection::OtherBookDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void Collection::OtherBookDataTable::AddOtherBookRow(ImportMSXML::Collection::OtherBookRow^  row) {
        this->Rows->Add(row);
    }
    
    inline ImportMSXML::Collection::OtherBookRow^  Collection::OtherBookDataTable::AddOtherBookRow(System::String^  Title, 
                System::String^  Author, System::String^  Publisher) {
        ImportMSXML::Collection::OtherBookRow^  rowOtherBookRow = (cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(3) {Title, Author, Publisher};
        rowOtherBookRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowOtherBookRow);
        return rowOtherBookRow;
    }
    
    inline ::System::Collections::IEnumerator^  Collection::OtherBookDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  Collection::OtherBookDataTable::Clone() {
        ImportMSXML::Collection::OtherBookDataTable^  cln = (cli::safe_cast<ImportMSXML::Collection::OtherBookDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  Collection::OtherBookDataTable::CreateInstance() {
        return (gcnew ImportMSXML::Collection::OtherBookDataTable());
    }
    
    inline ::System::Void Collection::OtherBookDataTable::InitVars() {
        this->columnTitle = __super::Columns[L"Title"];
        this->columnAuthor = __super::Columns[L"Author"];
        this->columnPublisher = __super::Columns[L"Publisher"];
    }
    
    inline ::System::Void Collection::OtherBookDataTable::InitClass() {
        this->columnTitle = (gcnew ::System::Data::DataColumn(L"Title", ::System::String::typeid, nullptr, ::System::Data::MappingType::Element));
        __super::Columns->Add(this->columnTitle);
        this->columnAuthor = (gcnew ::System::Data::DataColumn(L"Author", ::System::String::typeid, nullptr, ::System::Data::MappingType::Element));
        __super::Columns->Add(this->columnAuthor);
        this->columnPublisher = (gcnew ::System::Data::DataColumn(L"Publisher", ::System::String::typeid, nullptr, ::System::Data::MappingType::Element));
        __super::Columns->Add(this->columnPublisher);
        this->columnTitle->AllowDBNull = false;
        this->columnAuthor->AllowDBNull = false;
        this->columnPublisher->AllowDBNull = false;
    }
    
    inline ImportMSXML::Collection::OtherBookRow^  Collection::OtherBookDataTable::NewOtherBookRow() {
        return (cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  Collection::OtherBookDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew ImportMSXML::Collection::OtherBookRow(builder));
    }
    
    inline ::System::Type^  Collection::OtherBookDataTable::GetRowType() {
        return ImportMSXML::Collection::OtherBookRow::typeid;
    }
    
    inline ::System::Void Collection::OtherBookDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->OtherBookRowChanged(this, (gcnew ImportMSXML::Collection::OtherBookRowChangeEvent((cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void Collection::OtherBookDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->OtherBookRowChanging(this, (gcnew ImportMSXML::Collection::OtherBookRowChangeEvent((cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void Collection::OtherBookDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->OtherBookRowDeleted(this, (gcnew ImportMSXML::Collection::OtherBookRowChangeEvent((cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void Collection::OtherBookDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->OtherBookRowDeleting(this, (gcnew ImportMSXML::Collection::OtherBookRowChangeEvent((cli::safe_cast<ImportMSXML::Collection::OtherBookRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void Collection::OtherBookDataTable::RemoveOtherBookRow(ImportMSXML::Collection::OtherBookRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  Collection::OtherBookDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        ImportMSXML::Collection^  ds = (gcnew ImportMSXML::Collection());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"OtherBookDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline Collection::OtherBookRow::OtherBookRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableOtherBook = (cli::safe_cast<ImportMSXML::Collection::OtherBookDataTable^  >(this->Table));
    }
    
    inline System::String^  Collection::OtherBookRow::Title::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableOtherBook->TitleColumn]));
    }
    inline System::Void Collection::OtherBookRow::Title::set(System::String^  value) {
        this[this->tableOtherBook->TitleColumn] = value;
    }
    
    inline System::String^  Collection::OtherBookRow::Author::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableOtherBook->AuthorColumn]));
    }
    inline System::Void Collection::OtherBookRow::Author::set(System::String^  value) {
        this[this->tableOtherBook->AuthorColumn] = value;
    }
    
    inline System::String^  Collection::OtherBookRow::Publisher::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableOtherBook->PublisherColumn]));
    }
    inline System::Void Collection::OtherBookRow::Publisher::set(System::String^  value) {
        this[this->tableOtherBook->PublisherColumn] = value;
    }
    
    
    inline Collection::OtherBookRowChangeEvent::OtherBookRowChangeEvent(ImportMSXML::Collection::OtherBookRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline ImportMSXML::Collection::OtherBookRow^  Collection::OtherBookRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction Collection::OtherBookRowChangeEvent::Action::get() {
        return this->eventAction;
    }
}
